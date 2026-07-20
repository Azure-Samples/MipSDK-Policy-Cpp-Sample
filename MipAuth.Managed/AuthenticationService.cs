/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

using System.Collections.Concurrent;
using Microsoft.Identity.Client;

namespace MipAuth.Managed;

internal sealed record AuthenticationAccount(string Username, object NativeAccount);

internal interface IAuthenticationClient
{
    Task<IReadOnlyList<AuthenticationAccount>> GetAccountsAsync();

    Task<string?> AcquireTokenSilentAsync(
        string scope,
        AuthenticationAccount account,
        string? claims);

    Task<string> AcquireTokenInteractiveAsync(
        string scope,
        string username,
        string? claims);
}

internal sealed class MsalAuthenticationClient(IPublicClientApplication application)
    : IAuthenticationClient
{
    public async Task<IReadOnlyList<AuthenticationAccount>> GetAccountsAsync()
    {
        IEnumerable<IAccount> accounts = await application.GetAccountsAsync().ConfigureAwait(false);
        return accounts
            .Select(account => new AuthenticationAccount(account.Username, account))
            .ToArray();
    }

    public async Task<string?> AcquireTokenSilentAsync(
        string scope,
        AuthenticationAccount account,
        string? claims)
    {
        try
        {
            AcquireTokenSilentParameterBuilder builder = application
                .AcquireTokenSilent([scope], (IAccount)account.NativeAccount);
            if (claims is not null)
            {
                builder = builder.WithClaims(claims);
            }

            AuthenticationResult result = await builder.ExecuteAsync().ConfigureAwait(false);
            return result.AccessToken;
        }
        catch (MsalUiRequiredException)
        {
            return null;
        }
    }

    public async Task<string> AcquireTokenInteractiveAsync(
        string scope,
        string username,
        string? claims)
    {
        AcquireTokenInteractiveParameterBuilder builder = application
            .AcquireTokenInteractive([scope])
            .WithLoginHint(username)
            .WithUseEmbeddedWebView(false);
        if (claims is not null)
        {
            builder = builder.WithClaims(claims);
        }

        AuthenticationResult result = await builder.ExecuteAsync().ConfigureAwait(false);
        return result.AccessToken;
    }
}

internal static class AuthenticationService
{
    private sealed record ClientState(IAuthenticationClient Client, SemaphoreSlim Gate);

    private static readonly ConcurrentDictionary<string, Lazy<ClientState>> Clients =
        new(StringComparer.OrdinalIgnoreCase);

    internal static async Task<string> AcquireTokenAsync(ValidatedAuthRequest request)
    {
        string key = $"{request.Authority}\0{request.ClientId}";
        ClientState state = Clients.GetOrAdd(
            key,
            _ => new Lazy<ClientState>(
                () => CreateClientState(request),
                LazyThreadSafetyMode.ExecutionAndPublication)).Value;

        await state.Gate.WaitAsync().ConfigureAwait(false);
        try
        {
            return await AcquireTokenAsync(request, state.Client).ConfigureAwait(false);
        }
        finally
        {
            state.Gate.Release();
        }
    }

    internal static async Task<string> AcquireTokenAsync(
        ValidatedAuthRequest request,
        IAuthenticationClient client)
    {
        IReadOnlyList<AuthenticationAccount> accounts =
            await client.GetAccountsAsync().ConfigureAwait(false);
        foreach (AuthenticationAccount account in accounts.Where(
            account => string.Equals(
                account.Username,
                request.Username,
                StringComparison.OrdinalIgnoreCase)))
        {
            string? silentToken = await client.AcquireTokenSilentAsync(
                request.Scope,
                account,
                request.Claims).ConfigureAwait(false);
            if (!string.IsNullOrEmpty(silentToken))
            {
                return ValidateToken(silentToken);
            }
        }

        string interactiveToken = await client.AcquireTokenInteractiveAsync(
            request.Scope,
            request.Username,
            request.Claims).ConfigureAwait(false);
        return ValidateToken(interactiveToken);
    }

    private static ClientState CreateClientState(ValidatedAuthRequest request)
    {
        IPublicClientApplication application = PublicClientApplicationBuilder
            .Create(request.ClientId)
            .WithAuthority(request.Authority, validateAuthority: true)
            .WithRedirectUri("http://localhost")
            .Build();
        return new(new MsalAuthenticationClient(application), new SemaphoreSlim(1, 1));
    }

    private static string ValidateToken(string token)
    {
        if (string.IsNullOrEmpty(token) ||
            token.Length > 65535 ||
            token.Any(character => character is < '\x21' or > '\x7e'))
        {
            throw new InvalidOperationException("Invalid access token.");
        }

        return token;
    }
}
