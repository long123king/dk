# Quick Start Guide for CI/CD

## For Users - How to Install

1. Visit: https://github.com/long123king/dk/releases/latest
2. Download: `dk-v*.zip`
3. Extract and copy `dk.dll` to WinDbg extensions folder
4. In WinDbg: `.load dk.dll`
5. Verify: `!dk help`

## For Maintainers - How to Create a Release

### Quick Method (Recommended)

```bash
# Tag and push (GitHub Actions does the rest)
git tag -a v1.0.0 -m "Release v1.0.0"
git push origin v1.0.0
```

That's it! The release will be automatically:
- Built (Release x64)
- Packaged (DLL + PDB + LICENSE + README)
- Published to GitHub Releases

### Manual Trigger Method (For Testing)

You can also manually trigger a release from the GitHub UI:

1. Go to: https://github.com/long123king/dk/actions
2. Click "Release" workflow in the left sidebar
3. Click "Run workflow" button (top right)
4. Enter a tag name (e.g., `v0.0.1-test`)
5. Select your branch
6. Click "Run workflow"

This is useful for testing the release process without merging to main.

### What Happens Automatically

1. GitHub Actions detects the version tag
2. Sets up Windows build environment
3. Compiles the Release x64 build
4. Creates a ZIP file with all artifacts
5. Generates release notes
6. Creates a GitHub Release
7. Uploads the ZIP file

### Monitoring the Release

- Go to: https://github.com/long123king/dk/actions
- Click on the "Release" workflow
- Monitor progress
- Check result at: https://github.com/long123king/dk/releases

### Version Numbering

Use [Semantic Versioning](https://semver.org/):
- `v1.0.0` - Major release (breaking changes)
- `v1.1.0` - Minor release (new features)
- `v1.0.1` - Patch release (bug fixes)

### Before Creating a Release

- [ ] Code compiles successfully
- [ ] All tests pass (if applicable)
- [ ] Documentation is updated
- [ ] CHANGELOG.md is updated (optional)

## Build Status

Check these badges in README.md:
- **Build** - Shows if latest code compiles
- **Release** - Shows if latest release succeeded
- **Version** - Shows current release version

## Common Tasks

### Update Version Number
```bash
echo "VERSION=1.0.1" > VERSION
git add VERSION
git commit -m "Bump version to 1.0.1"
```

### Delete a Release
1. Go to GitHub Releases page
2. Click on the release
3. Delete the release
4. Delete the tag: `git push --delete origin v1.0.0`

### Re-create a Failed Release
1. Delete the existing tag and release
2. Fix any issues in code
3. Create a new tag and push

## Files Reference

- `.github/workflows/build.yml` - CI build configuration
- `.github/workflows/release.yml` - Release automation
- `RELEASE.md` - Detailed release documentation
- `CHANGELOG.md` - Version history tracking
- `CI_CD_SUMMARY.md` - Complete overview
- `VERSION` - Current version number

## Need Help?

- Review `RELEASE.md` for detailed instructions
- Check `CI_CD_SUMMARY.md` for technical details
- View workflow logs in GitHub Actions
- Open an issue if problems persist

---

**Pro Tip**: Always test the build locally before creating a release!
