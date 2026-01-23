# Release Process

This document describes how to create a new release of the dk WinDbg extension.

## Automated Release Process

The project uses GitHub Actions to automatically build and release the extension when a version tag is pushed.

### Creating a New Release

1. **Update Version Information** (if using VERSION file):
   ```bash
   # Edit the VERSION file with the new version number
   echo "VERSION=1.0.1" > VERSION
   git add VERSION
   git commit -m "Bump version to 1.0.1"
   ```

2. **Create and Push a Git Tag**:
   ```bash
   # Create a new tag (e.g., v1.0.1)
   git tag -a v1.0.1 -m "Release version 1.0.1"
   
   # Push the tag to GitHub
   git push origin v1.0.1
   ```

3. **Automated Build and Release**:
   - GitHub Actions will automatically:
     - Build the Release x64 version
     - Create a ZIP file with the DLL, PDB, LICENSE, and README
     - Generate release notes
     - Create a GitHub Release with the artifacts
     - Attach the built artifacts to the release

4. **Verify the Release**:
   - Go to https://github.com/long123king/dk/releases
   - Verify the new release appears with the correct artifacts
   - Download and test the release build

## Manual Release Process (if needed)

If you need to create a release manually:

1. **Build the Release Version**:
   ```bash
   # Open Visual Studio Developer Command Prompt
   msbuild dk.sln /p:Configuration=Release /p:Platform=x64 /m
   ```

2. **Package the Artifacts**:
   ```powershell
   # Create a directory for release files
   mkdir release-v1.0.0
   
   # Copy files
   copy x64\Release\dk.dll release-v1.0.0\
   copy x64\Release\dk.pdb release-v1.0.0\
   copy LICENSE release-v1.0.0\
   copy README.md release-v1.0.0\
   
   # Create ZIP file
   Compress-Archive -Path release-v1.0.0\* -DestinationPath dk-v1.0.0-x64.zip
   ```

3. **Create GitHub Release**:
   - Go to https://github.com/long123king/dk/releases/new
   - Create a new tag (e.g., v1.0.0)
   - Fill in the release title and description
   - Upload the ZIP file
   - Publish the release

## Continuous Integration

The project also has a build workflow that runs on every push and pull request:
- Builds both Debug and Release configurations
- Uploads build artifacts for inspection
- Ensures code compiles successfully

## Version Numbering

This project follows [Semantic Versioning](https://semver.org/):
- **MAJOR**: Incompatible API changes
- **MINOR**: Add functionality in a backward compatible manner
- **PATCH**: Backward compatible bug fixes

Examples:
- `v1.0.0` - Initial stable release
- `v1.0.1` - Bug fix release
- `v1.1.0` - New features, backward compatible
- `v2.0.0` - Breaking changes

## Release Checklist

Before creating a release:
- [ ] All tests pass (if applicable)
- [ ] Code builds successfully in Release configuration
- [ ] README.md is up to date
- [ ] Version number is updated (if using VERSION file)
- [ ] CHANGELOG or release notes are prepared
- [ ] All planned features/fixes are included

## Troubleshooting

### Build Failures
- Ensure Windows SDK is installed
- Check that all dependencies are available
- Review build logs in GitHub Actions

### Release Not Created
- Verify the tag format starts with 'v' (e.g., v1.0.0)
- Check GitHub Actions permissions
- Review the release workflow logs

## Resources

- [GitHub Actions Documentation](https://docs.github.com/en/actions)
- [MSBuild Documentation](https://docs.microsoft.com/en-us/visualstudio/msbuild/msbuild)
- [WinDbg Extension Development](https://docs.microsoft.com/en-us/windows-hardware/drivers/debugger/)
