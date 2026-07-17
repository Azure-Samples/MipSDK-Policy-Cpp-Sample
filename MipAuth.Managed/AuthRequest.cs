/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

using System.Text.Json;
using System.Text.RegularExpressions;

namespace MipAuth.Managed;

internal sealed record ValidatedAuthRequest(
    string Username,
    string ClientId,
    string Authority,
    string Scope,
    string? Claims);

internal static partial class AuthRequestValidator
{
    private static readonly HashSet<string> AuthorityHosts = new(StringComparer.OrdinalIgnoreCase)
    {
        "login.chinacloudapi.cn",
        "login.microsoftonline.com",
        "login.microsoftonline.de",
        "login.microsoftonline.us",
        "login.partner.microsoftonline.cn",
        "login.windows.net",
    };

    private static readonly HashSet<string> ResourceHosts = new(StringComparer.OrdinalIgnoreCase)
    {
        "api.aadrm.cn",
        "api.aadrm.com",
        "api.aadrm.de",
        "api.aadrm.us",
        "api.azurerms.com",
        "syncservice.o365syncservice.com",
    };

    [GeneratedRegex(@"^[A-Za-z0-9](?:[A-Za-z0-9._-]{0,252}[A-Za-z0-9])?$",
        RegexOptions.CultureInvariant)]
    private static partial Regex TenantPattern();

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
            !username.Contains('@', StringComparison.Ordinal) ||
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
        Uri uri = ParseHttpsUri(authority, AuthorityHosts);
        string path = uri.AbsolutePath;
        if (path.EndsWith('/'))
        {
            path = path[..^1];
        }

        if (path.Length < 2 ||
            path[0] != '/' ||
            path.IndexOf('/', 1) >= 0 ||
            !TenantPattern().IsMatch(path[1..]))
        {
            throw new ArgumentException("Invalid authority.", nameof(authority));
        }

        string tenant = path[1..];
        if (tenant.Equals("common", StringComparison.OrdinalIgnoreCase) ||
            tenant.Equals("organizations", StringComparison.OrdinalIgnoreCase))
        {
            tenant = GetTenantDomain(username);
        }

        return $"https://{uri.IdnHost.ToLowerInvariant()}/{tenant}";
    }

    private static string ValidateResource(string resource)
    {
        Uri uri = ParseHttpsUri(resource, ResourceHosts);
        string path = uri.AbsolutePath;
        if (path is not ("" or "/" or "/.default" or "/.default/"))
        {
            throw new ArgumentException("Invalid resource.", nameof(resource));
        }

        return $"https://{uri.IdnHost.ToLowerInvariant()}/.default";
    }

    private static Uri ParseHttpsUri(string value, HashSet<string> allowedHosts)
    {
        if (!Uri.TryCreate(value, UriKind.Absolute, out Uri? uri) ||
            !string.Equals(uri.Scheme, Uri.UriSchemeHttps, StringComparison.OrdinalIgnoreCase) ||
            !allowedHosts.Contains(uri.IdnHost) ||
            !string.IsNullOrEmpty(uri.UserInfo) ||
            (!uri.IsDefaultPort && uri.Port != 443) ||
            !string.IsNullOrEmpty(uri.Query) ||
            !string.IsNullOrEmpty(uri.Fragment) ||
            value.Contains('\\'))
        {
            throw new ArgumentException("Invalid URI.", nameof(value));
        }

        return uri;
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
        if (domain.Length is 0 or > 253 ||
            domain.Any(character => character > 0x7f) ||
            domain.Split('.').Any(label =>
                label.Length is 0 or > 63 ||
                !char.IsAsciiLetterOrDigit(label[0]) ||
                !char.IsAsciiLetterOrDigit(label[^1]) ||
                label.Any(character =>
                    !char.IsAsciiLetterOrDigit(character) && character != '-')))
        {
            throw new ArgumentException("Invalid username.", nameof(username));
        }

        return domain;
    }
}
