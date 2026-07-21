/*
 * Copyright (c) Microsoft Corporation. All rights reserved.
 * Licensed under the MIT License.
 */

namespace MipAuth.Managed.Tests;

public sealed class AuthRequestValidatorTests
{
    [Fact]
    public void ValidateNormalizesAuthorityAndDynamicDefaultScope()
    {
        ValidatedAuthRequest request = AuthRequestValidator.Validate(
            "user@contoso.com",
            "00000000-0000-0000-0000-000000000001",
            "https://LOGIN.MICROSOFTONLINE.COM/contoso.onmicrosoft.com/",
            "https://custom.policy.endpoint.contoso.com/",
            """{"access_token":{"xms_cc":{"values":["cp1"]}}}""");

        Assert.Equal(
            "https://LOGIN.MICROSOFTONLINE.COM/contoso.onmicrosoft.com",
            request.Authority);
        Assert.Equal("https://custom.policy.endpoint.contoso.com/.default", request.Scope);
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
            "https://policy.contoso.com",
            null);

        Assert.Equal(
            "https://login.microsoftonline.com/contoso.onmicrosoft.com",
            request.Authority);
    }

    [Theory]
    [InlineData("")]
    [InlineData("not-a-uri")]
    [InlineData("http://login.microsoftonline.com/common")]
    [InlineData("https://login.microsoftonline.com/common?x=1")]
    public void ValidateRejectsInvalidAuthority(string authority)
    {
        Assert.Throws<ArgumentException>(() => AuthRequestValidator.Validate(
            "user@contoso.com",
            "00000000-0000-0000-0000-000000000001",
            authority,
            "https://policy.contoso.com",
            null));
    }

    [Fact]
    public void ValidateRejectsEmptyResource()
    {
        Assert.Throws<ArgumentException>(() => AuthRequestValidator.Validate(
            "user@contoso.com",
            "00000000-0000-0000-0000-000000000001",
            "https://login.microsoftonline.com/common",
            "",
            null));
    }

    [Fact]
    public void ValidateRejectsNonObjectClaims()
    {
        Assert.Throws<ArgumentException>(() => AuthRequestValidator.Validate(
            "user@contoso.com",
            "00000000-0000-0000-0000-000000000001",
            "https://login.microsoftonline.com/common",
            "https://policy.contoso.com",
            "[]"));
    }

    [Theory]
    [InlineData("user@bad..example")]
    [InlineData("user@-bad.example")]
    public void ValidateRejectsInvalidDerivedTenant(string username)
    {
        ArgumentException exception = Assert.Throws<ArgumentException>(() => AuthRequestValidator.Validate(
                username,
                "00000000-0000-0000-0000-000000000001",
                "https://login.microsoftonline.com/common",
                "https://policy.contoso.com",
                null));
        Assert.Contains("tenant domain", exception.Message, StringComparison.OrdinalIgnoreCase);
    }
}
