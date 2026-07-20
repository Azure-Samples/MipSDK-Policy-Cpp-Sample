/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

namespace MipAuth.Managed.Tests;

public sealed class AuthenticationServiceTests
{
    [Fact]
    public async Task AcquireTokenUsesUsernameMatchedSilentTokenFirst()
    {
        ValidatedAuthRequest request = CreateRequest();
        FakeAuthenticationClient client = new(
            [
                new("other@contoso.com", new object()),
                new("USER@CONTOSO.COM", new object()),
            ],
            silentToken: "silent-token",
            interactiveToken: "interactive-token");

        string token = await AuthenticationService.AcquireTokenAsync(request, client);

        Assert.Equal("silent-token", token);
        Assert.Equal(1, client.SilentCalls);
        Assert.Equal(0, client.InteractiveCalls);
        Assert.Equal(request.Claims, client.LastClaims);
    }

    [Fact]
    public async Task AcquireTokenFallsBackToSystemBrowserFlow()
    {
        ValidatedAuthRequest request = CreateRequest();
        FakeAuthenticationClient client = new(
            [new("user@contoso.com", new object())],
            silentToken: null,
            interactiveToken: "interactive-token");

        string token = await AuthenticationService.AcquireTokenAsync(request, client);

        Assert.Equal("interactive-token", token);
        Assert.Equal(1, client.SilentCalls);
        Assert.Equal(1, client.InteractiveCalls);
        Assert.Equal(request.Claims, client.LastClaims);
    }

    private static ValidatedAuthRequest CreateRequest() => AuthRequestValidator.Validate(
        "user@contoso.com",
        "00000000-0000-0000-0000-000000000001",
        "https://login.microsoftonline.com/common",
        "https://api.aadrm.com",
        """{"access_token":{"xms_cc":{"values":["cp1"]}}}""");

    private sealed class FakeAuthenticationClient(
        IReadOnlyList<AuthenticationAccount> accounts,
        string? silentToken,
        string interactiveToken) : IAuthenticationClient
    {
        internal int SilentCalls { get; private set; }
        internal int InteractiveCalls { get; private set; }
        internal string? LastClaims { get; private set; }

        public Task<IReadOnlyList<AuthenticationAccount>> GetAccountsAsync() =>
            Task.FromResult(accounts);

        public Task<string?> AcquireTokenSilentAsync(
            string scope,
            AuthenticationAccount account,
            string? claims)
        {
            SilentCalls++;
            LastClaims = claims;
            return Task.FromResult(silentToken);
        }

        public Task<string> AcquireTokenInteractiveAsync(
            string scope,
            string username,
            string? claims)
        {
            InteractiveCalls++;
            LastClaims = claims;
            return Task.FromResult(interactiveToken);
        }
    }
}
