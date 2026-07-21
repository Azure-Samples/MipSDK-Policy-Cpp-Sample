---
page_type: sample
languages:
- cpp
- csharp
products:
- m365
- office-365
description: "This application demonstrates using the MIP SDK Policy API to list available labels."
urlFragment: MipSDK-Policy-Cpp-Sample
---

# MipSDK-Policy-Cpp-Sample

## Summary

This application demonstrates using the MIP SDK Policy API to list available labels. It prompts the user to input a label, computes the actions that should be taken, and outputs the metadata that would be applied to the document.

The application demonstrates:

- Initializing the `PolicyProfile`
- Adding the `PolicyEngine`
- Creating a `PolicyEngine::Handler`
- Implementing `ExecutionState` and providing options to compute actions
- Looping on `ComputeActions()` and updating the execution state
- Hosting a framework-dependent .NET 8 MSAL component in-process

## Prerequisites

- Visual Studio 2022 with the Visual C++ v143 and .NET 8 development tools
- The .NET 8 runtime
- NuGet package restore access

The native sample remains aligned to Microsoft Information Protection Policy SDK **1.18.124**. Authentication uses MSAL.NET **4.86.1**.

## Build

1. Clone the repository and open `mipsdk-policyapi-cpp-sample-basic.sln`.
2. Select **x64** and the desired build configuration.
3. Restore NuGet packages.
4. Build the solution.

The solution builds `MipAuth.Managed` before the native projects and copies its DLL, dependency manifest, runtime configuration, MSAL dependencies, and `nethost.dll` beside the native executables. The application is framework-dependent and uses the installed .NET 8 runtime.

## Authentication architecture

The native process locates `hostfxr` through the official `nethost` API, initializes .NET once from `MipAuth.Managed.runtimeconfig.json`, and loads `MipAuth.Managed.dll` from the executable directory. Authentication crosses a versioned, blittable component ABI with caller-owned UTF-8 token and error buffers of at least 64 KiB. Tokens and errors are returned through memory and integer return codes only.

The managed component:

- passes authority/resource/claims values directly from each MIP SDK OAuth challenge;
- normalizes the challenge resource to one `/.default` scope and uses that same scope for both silent and interactive MSAL requests;
- checks the in-process MSAL cache for an exact, case-insensitive username match before interactive fallback;
- normalizes `/common` and `/organizations` authorities to a tenant-specific authority derived from the signed-in username domain; and
- never accepts passwords, parses JWTs, or logs tokens.

No authentication environment variables or external helper processes are required.

## Create a Microsoft Entra app registration

1. In the Azure portal, open **Microsoft Entra ID** > **App registrations**.
2. Create a native/public client registration.
3. Add the delegated **UnifiedPolicy.User.Read** permission from **Microsoft Information Protection Sync Service** and grant the required consent.
4. Open `main.cpp` and replace **YOUR APPLICATION ID** and **YOUR USER UPN**. The username is only a login hint and cache selector.

### Set Redirect URI

1. Select **Authentication**.
2. Select **Add a platform**.
3. Select **Mobile and desktop applications**
4. Add the default native client redirect URI **http://localhost**.
5. Under **Settings** set **Allow public client flows** to **Enabled**.
6. Click **Save**.

## Run

Press F5. Authentication first attempts a username-matched silent MSAL request. If user interaction is required, the default system browser opens. The console application then displays labels available to the user.

## Validation

- Run `dotnet test MipAuth.Managed.Tests\MipAuth.Managed.Tests.csproj -c Release`.
- Run `x64\Release\MipAuth.NativeSmoke.exe` after the Release x64 solution build.

The smoke executable loads the managed component through `nethost`/`hostfxr`, invokes the native ABI with a deliberately invalid request, and verifies that the managed validation error is returned through the caller-owned error buffer.

## Resources

- [Microsoft Information Protection documentation](https://aka.ms/mipsdkdocs)
