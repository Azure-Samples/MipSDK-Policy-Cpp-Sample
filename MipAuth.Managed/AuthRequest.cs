/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

using System.Text.Json;

namespace MipAuth.Managed;

internal sealed record ValidatedAuthRequest(
    string Username,
    string ClientId,
    string Authority,
    string Scope,
    string? Claims);

internal static partial class AuthRequestValidator
{
    internal static ValidatedAuthRequest Validate(
        string username,
        string clientId,
        string authority,
        string resource,
        string? claims)
    {
        ValidateUsername(username);
        ValidateClientId(clientId);
        string normalizedAuthority = ValidateAuthority(authority, username);
        string scope = ValidateResource(resource);
        string? normalizedClaims = ValidateClaims(claims);
        return new(username, clientId, normalizedAuthority, scope, normalizedClaims);
    }
    private static void ValidateUsername(string username)
    {
        if (username.Length is 0 or > 320 ||
            username.Any(character => char.IsWhiteSpace(character) || char.IsControl(character)))
        {
            throw new ArgumentException("Invalid username.", nameof(username));
        }
    }

    private static void ValidateClientId(string clientId)
    {
        if (!Guid.TryParse(clientId, out Guid parsed) ||
            parsed == Guid.Empty ||
            !string.Equals(parsed.ToString("D"), clientId, StringComparison.OrdinalIgnoreCase))
        {
            throw new ArgumentException("Invalid client ID.", nameof(clientId));
        }
    }

    private static string ValidateAuthority(string authority, string username)
    {
        if (!Uri.TryCreate(authority, UriKind.Absolute, out Uri? uri) ||
            !string.Equals(uri.Scheme, Uri.UriSchemeHttps, StringComparison.OrdinalIgnoreCase) ||
            !string.IsNullOrEmpty(uri.UserInfo) ||
            !string.IsNullOrEmpty(uri.Query) ||
            !string.IsNullOrEmpty(uri.Fragment) ||
            authority.Contains('\\'))
        {
            throw new ArgumentException("Invalid authority.", nameof(authority));
        }

        string path = uri.AbsolutePath.Trim('/');
        if (path.Length == 0)
        {
            throw new ArgumentException("Invalid authority.", nameof(authority));
        }

        if (path.Equals("common", StringComparison.OrdinalIgnoreCase) ||
            path.Equals("organizations", StringComparison.OrdinalIgnoreCase))
        {
            string tenant = GetTenantDomain(username);
            return $"https://{uri.IdnHost.ToLowerInvariant()}/{tenant}";
        }

        return authority.TrimEnd('/');
    }

    private static string ValidateResource(string resource)
    {
        if (string.IsNullOrWhiteSpace(resource))
        {
            throw new ArgumentException("Resource is required.", nameof(resource));
        }

        string scope = resource.TrimEnd('/');
        if (scope.EndsWith("/.default", StringComparison.OrdinalIgnoreCase))
        {
            return scope;
        }

        return $"{scope}/.default";
    }

    private static string? ValidateClaims(string? claims)
    {
        if (string.IsNullOrEmpty(claims))
        {
            return null;
        }

        if (claims.Length > 64 * 1024)
        {
            throw new ArgumentException("Invalid claims.", nameof(claims));
        }

        using JsonDocument document = JsonDocument.Parse(claims);
        if (document.RootElement.ValueKind != JsonValueKind.Object)
        {
            throw new ArgumentException("Invalid claims.", nameof(claims));
        }

        return claims;
    }

    private static string GetTenantDomain(string username)
    {
        int separator = username.LastIndexOf('@');
        string domain = separator > 0 && separator < username.Length - 1
            ? username[(separator + 1)..].ToLowerInvariant()
            : string.Empty;
        if (!IsValidDnsHost(domain))
        {
            throw new ArgumentException("Username must contain a valid tenant domain.", nameof(username));
        }

        return domain;
    }

    private static bool IsValidDnsHost(string host)
    {
        if (host.Length is 0 or > 253 || host.Any(character => character > 0x7f))
        {
            return false;
        }

        foreach (string label in host.Split('.'))
        {
            if (label.Length is 0 or > 63 ||
                !char.IsAsciiLetterOrDigit(label[0]) ||
                !char.IsAsciiLetterOrDigit(label[^1]) ||
                label.Any(character =>
                    !char.IsAsciiLetterOrDigit(character) && character != '-'))
            {
                return false;
            }
        }

        return true;
    }
}
