/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using System.Text;
using System.Text.Json;
using Microsoft.Identity.Client;

namespace MipAuth.Managed;

internal enum MipAuthResult
{
    Success = 0,
    InvalidRequest = 1,
    ValidationFailed = 2,
    AuthenticationFailed = 3,
    BufferTooSmall = 4,
    InternalError = 5,
}

[StructLayout(LayoutKind.Sequential, Pack = 8)]
internal struct MipAuthRequestV1
{
    internal const uint CurrentVersion = 1;
    internal const uint MinimumBufferSize = 64 * 1024;

    internal uint Version;
    internal uint Size;
    internal IntPtr Username;
    internal uint UsernameLength;
    internal IntPtr ClientId;
    internal uint ClientIdLength;
    internal IntPtr Authority;
    internal uint AuthorityLength;
    internal IntPtr Resource;
    internal uint ResourceLength;
    internal IntPtr Claims;
    internal uint ClaimsLength;
    internal IntPtr TokenBuffer;
    internal uint TokenBufferSize;
    internal uint TokenLength;
    internal IntPtr ErrorBuffer;
    internal uint ErrorBufferSize;
    internal uint ErrorLength;
}

public static unsafe class NativeEntryPoint
{
    private static readonly UTF8Encoding StrictUtf8 = new(false, true);

    [UnmanagedCallersOnly(EntryPoint = "AcquireToken")]
    public static int AcquireToken(IntPtr request, int requestSize) => Invoke(request, requestSize);

    internal static int Invoke(IntPtr requestPointer, int requestSize)
    {
        if (requestPointer == IntPtr.Zero ||
            requestSize < sizeof(MipAuthRequestV1))
        {
            return (int)MipAuthResult.InvalidRequest;
        }

        MipAuthRequestV1* request = (MipAuthRequestV1*)requestPointer;
        request->TokenLength = 0;
        request->ErrorLength = 0;

        if (request->Version != MipAuthRequestV1.CurrentVersion ||
            request->Size < sizeof(MipAuthRequestV1) ||
            request->Size > requestSize ||
            request->TokenBuffer == IntPtr.Zero ||
            request->ErrorBuffer == IntPtr.Zero ||
            request->TokenBufferSize < MipAuthRequestV1.MinimumBufferSize ||
            request->ErrorBufferSize < MipAuthRequestV1.MinimumBufferSize)
        {
            return (int)MipAuthResult.InvalidRequest;
        }

        try
        {
            ValidatedAuthRequest validated = AuthRequestValidator.Validate(
                ReadRequiredUtf8(request->Username, request->UsernameLength),
                ReadRequiredUtf8(request->ClientId, request->ClientIdLength),
                ReadRequiredUtf8(request->Authority, request->AuthorityLength),
                ReadRequiredUtf8(request->Resource, request->ResourceLength),
                ReadOptionalUtf8(request->Claims, request->ClaimsLength));

            string token = AuthenticationService.AcquireTokenAsync(validated)
                .GetAwaiter()
                .GetResult();
            if (!WriteUtf8(request->TokenBuffer, request->TokenBufferSize, token, out uint length))
            {
                WriteError(request, "Authentication token exceeds the caller buffer.");
                return (int)MipAuthResult.BufferTooSmall;
            }

            request->TokenLength = length;
            return (int)MipAuthResult.Success;
        }
        catch (ArgumentException exception)
        {
            WriteError(request, exception.Message);
            return (int)MipAuthResult.ValidationFailed;
        }
        catch (JsonException exception)
        {
            WriteError(request, exception.Message);
            return (int)MipAuthResult.ValidationFailed;
        }
        catch (MsalException)
        {
            WriteError(request, "Authentication failed.");
            return (int)MipAuthResult.AuthenticationFailed;
        }
        catch
        {
            WriteError(request, "Authentication failed.");
            return (int)MipAuthResult.InternalError;
        }
    }

    private static string ReadRequiredUtf8(IntPtr pointer, uint length)
    {
        if (pointer == IntPtr.Zero || length is 0 or > 64 * 1024)
        {
            throw new ArgumentException("Invalid UTF-8 input.");
        }

        return ReadUtf8(pointer, length);
    }

    private static string? ReadOptionalUtf8(IntPtr pointer, uint length)
    {
        if (length == 0)
        {
            if (pointer != IntPtr.Zero)
            {
                return string.Empty;
            }
            return null;
        }

        if (pointer == IntPtr.Zero || length > 64 * 1024)
        {
            throw new ArgumentException("Invalid UTF-8 input.");
        }

        return ReadUtf8(pointer, length);
    }

    private static string ReadUtf8(IntPtr pointer, uint length)
    {
        ReadOnlySpan<byte> bytes = new((void*)pointer, checked((int)length));
        string value = StrictUtf8.GetString(bytes);
        if (value.Contains('\0', StringComparison.Ordinal))
        {
            throw new ArgumentException("Invalid UTF-8 input.");
        }
        return value;
    }

    private static bool WriteUtf8(
        IntPtr buffer,
        uint bufferSize,
        string value,
        out uint length)
    {
        int byteCount = StrictUtf8.GetByteCount(value);
        if ((ulong)byteCount + 1 > bufferSize)
        {
            length = 0;
            return false;
        }

        Span<byte> destination = new((void*)buffer, checked((int)bufferSize));
        int bytesWritten = StrictUtf8.GetBytes(value, destination);
        destination[bytesWritten] = 0;
        length = checked((uint)bytesWritten);
        return true;
    }

    private static void WriteError(MipAuthRequestV1* request, string message)
    {
        if (WriteUtf8(
            request->ErrorBuffer,
            request->ErrorBufferSize,
            message,
            out uint errorLength))
        {
            request->ErrorLength = errorLength;
        }
    }
}
