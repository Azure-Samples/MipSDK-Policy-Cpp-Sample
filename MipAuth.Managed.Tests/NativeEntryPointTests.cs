/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

using System.Runtime.InteropServices;
using System.Text;

namespace MipAuth.Managed.Tests;

public sealed class NativeEntryPointTests
{
    [Fact]
    public void InvokeReturnsValidationErrorThroughCallerBuffer()
    {
        using NativeRequest request = new(
            "invalid-user",
            "00000000-0000-0000-0000-000000000001",
            "https://login.microsoftonline.com/common",
            "https://api.aadrm.com",
            null);

        int result = NativeEntryPoint.Invoke(
            request.Pointer,
            Marshal.SizeOf<MipAuthRequestV1>());
        MipAuthRequestV1 response = Marshal.PtrToStructure<MipAuthRequestV1>(request.Pointer);

        Assert.Equal((int)MipAuthResult.ValidationFailed, result);
        Assert.Equal((uint)0, response.TokenLength);
        string? error = Marshal.PtrToStringUTF8(
            response.ErrorBuffer,
            checked((int)response.ErrorLength));
        Assert.NotNull(error);
        Assert.Contains("tenant domain", error, StringComparison.OrdinalIgnoreCase);
    }

    [Fact]
    public void AbiLayoutIsStableOnX64()
    {
        if (IntPtr.Size == 8)
        {
            Assert.Equal(120, Marshal.SizeOf<MipAuthRequestV1>());
        }
    }

    private sealed class NativeRequest : IDisposable
    {
        private readonly List<IntPtr> allocations = [];

        internal NativeRequest(
            string username,
            string clientId,
            string authority,
            string resource,
            string? claims)
        {
            IntPtr claimsPointer = IntPtr.Zero;
            uint claimsLength = 0;
            if (claims is not null)
            {
                claimsPointer = AllocateString(claims, out claimsLength);
            }

            MipAuthRequestV1 request = new()
            {
                Version = MipAuthRequestV1.CurrentVersion,
                Size = checked((uint)Marshal.SizeOf<MipAuthRequestV1>()),
                Username = AllocateString(username, out uint usernameLength),
                UsernameLength = usernameLength,
                ClientId = AllocateString(clientId, out uint clientIdLength),
                ClientIdLength = clientIdLength,
                Authority = AllocateString(authority, out uint authorityLength),
                AuthorityLength = authorityLength,
                Resource = AllocateString(resource, out uint resourceLength),
                ResourceLength = resourceLength,
                Claims = claimsPointer,
                ClaimsLength = claimsLength,
                TokenBuffer = Allocate(MipAuthRequestV1.MinimumBufferSize),
                TokenBufferSize = MipAuthRequestV1.MinimumBufferSize,
                ErrorBuffer = Allocate(MipAuthRequestV1.MinimumBufferSize),
                ErrorBufferSize = MipAuthRequestV1.MinimumBufferSize,
            };

            Pointer = Allocate(checked((uint)Marshal.SizeOf<MipAuthRequestV1>()));
            Marshal.StructureToPtr(request, Pointer, false);
        }

        internal IntPtr Pointer { get; }

        public void Dispose()
        {
            foreach (IntPtr allocation in allocations)
            {
                Marshal.FreeHGlobal(allocation);
            }
        }

        private IntPtr AllocateString(string value, out uint length)
        {
            byte[] bytes = Encoding.UTF8.GetBytes(value);
            IntPtr pointer = Allocate(checked((uint)bytes.Length + 1));
            Marshal.Copy(bytes, 0, pointer, bytes.Length);
            length = checked((uint)bytes.Length);
            return pointer;
        }

        private IntPtr Allocate(uint size)
        {
            IntPtr pointer = Marshal.AllocHGlobal(checked((int)size));
            allocations.Add(pointer);
            return pointer;
        }
    }
}
