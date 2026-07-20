/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

namespace MipAuth.Managed.Tests;

public sealed class AuthRequestValidatorTests
{
    [Fact]
    public void ValidateNormalizesAuthorityAndDefaultScope()
    {
        ValidatedAuthRequest request = AuthRequestValidator.Validate(
            "user@contoso.com",
            "00000000-0000-0000-0000-000000000001",
            "https://LOGIN.MICROSOFTONLINE.COM/contoso.onmicrosoft.com/",
            "https://API.AADRm.COM/.default/",
            """{"access_token":{"xms_cc":{"values":["cp1"]}}}""");

        Assert.Equal(
            "https://login.microsoftonline.com/contoso.onmicrosoft.com",
            request.Authority);
        Assert.Equal("https://api.aadrm.com/.default", request.Scope);
        Assert.NotNull(request.Claims);
    }

    [Theory]
    [InlineData("common")]
    [InlineData("organizations")]
    public void ValidateUsesUsernameDomainForTenantIndependentAuthorities(string tenant)
    {
        ValidatedAuthRequest request = AuthRequestValidator.Validate(
            "user@contoso.onmicrosoft.com",
            "00000000-0000-0000-0000-000000000001",
            $"https://login.microsoftonline.com/{tenant}",
            "https://api.aadrm.com",
            null);

        Assert.Equal(
            "https://login.microsoftonline.com/contoso.onmicrosoft.com",
            request.Authority);
    }

    [Theory]
    [InlineData("http://login.microsoftonline.com/common")]
    [InlineData("https://evil.example/common")]
    [InlineData("https://login.microsoftonline.com/common/oauth2")]
    [InlineData("https://login.microsoftonline.com/common?x=1")]
    public void ValidateRejectsInvalidAuthority(string authority)
    {
        Assert.Throws<ArgumentException>(() => AuthRequestValidator.Validate(
            "user@contoso.com",
            "00000000-0000-0000-0000-000000000001",
            authority,
            "https://api.aadrm.com",
            null));
    }

    [Theory]
    [InlineData("https://example.com")]
    [InlineData("https://api.aadrm.com/path")]
    [InlineData("http://api.aadrm.com")]
    public void ValidateRejectsInvalidResource(string resource)
    {
        Assert.Throws<ArgumentException>(() => AuthRequestValidator.Validate(
            "user@contoso.com",
            "00000000-0000-0000-0000-000000000001",
            "https://login.microsoftonline.com/common",
            resource,
            null));
    }

    [Fact]
    public void ValidateRejectsNonObjectClaims()
    {
        Assert.Throws<ArgumentException>(() => AuthRequestValidator.Validate(
            "user@contoso.com",
            "00000000-0000-0000-0000-000000000001",
            "https://login.microsoftonline.com/common",
            "https://api.aadrm.com",
            "[]"));
    }

    [Theory]
    [InlineData("user@bad..example")]
    [InlineData("user@-bad.example")]
    public void ValidateRejectsInvalidDerivedTenant(string username)
    {
        Assert.Throws<ArgumentException>(() => AuthRequestValidator.Validate(
            username,
            "00000000-0000-0000-0000-000000000001",
            "https://login.microsoftonline.com/common",
            "https://api.aadrm.com",
            null));
    }
}
