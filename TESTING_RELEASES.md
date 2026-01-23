# Testing the Release Build Process

This guide explains how to test the release build process without merging to the main branch.

## Method 1: Test Release Workflow with Tags on Your Branch (Recommended)

The release workflow can be triggered on **any branch** by pushing a tag. You don't need to merge to main first.

### Step-by-Step Instructions

#### Step 1: Ensure You're on Your Branch
```bash
# Check your current branch
git branch

# You should see something like:
# * copilot/add-ci-cd-integration
```

#### Step 2: Create a Test Tag
Create a tag with a test version number. Use a format like `v0.0.1-test` to indicate it's a test release:

```bash
# Create an annotated tag
git tag -a v0.0.1-test -m "Test release build"
```

#### Step 3: Push the Tag to GitHub
```bash
# Push the tag to trigger the release workflow
git push origin v0.0.1-test
```

#### Step 4: Monitor the Workflow
1. Go to GitHub: https://github.com/long123king/dk/actions
2. You'll see a "Release" workflow running
3. Click on it to see the build progress
4. Wait for it to complete (usually 5-10 minutes)

#### Step 5: Check the Release
1. Go to: https://github.com/long123king/dk/releases
2. You'll see a new release named `v0.0.1-test`
3. Download the `dk-v0.0.1-test-x64.zip` file
4. Extract and verify the contents:
   - `dk.dll` (Release x64 build)
   - `dk.pdb` (Debug symbols)
   - `LICENSE`
   - `README.md`

#### Step 6: Clean Up (Optional)
After testing, you can delete the test release and tag:

```bash
# Delete the tag locally
git tag -d v0.0.1-test

# Delete the tag from GitHub
git push --delete origin v0.0.1-test
```

Then manually delete the release from GitHub:
1. Go to https://github.com/long123king/dk/releases
2. Click on the test release
3. Click "Delete" button

---

## Method 2: Test Build Workflow on Pull Request

The build workflow automatically runs when you create or update a pull request.

### Step-by-Step Instructions

#### Step 1: Create a Pull Request
1. Go to: https://github.com/long123king/dk/pulls
2. Click "New pull request"
3. Select your branch (copilot/add-ci-cd-integration) as the source
4. Select main/master as the target
5. Click "Create pull request"

#### Step 2: Monitor the Build
1. On the PR page, you'll see "Checks" running
2. Click "Details" next to the "Build" check
3. Watch the build process for both Debug and Release configurations

#### Step 3: Download Build Artifacts
1. After the build completes, go to the workflow run
2. Scroll down to "Artifacts" section
3. Download:
   - `dk-Debug-x64` (contains Debug build)
   - `dk-Release-x64` (contains Release build)
4. Extract and verify the DLL and PDB files

---

## Method 3: Manual Workflow Dispatch (If Added)

If the workflow has a `workflow_dispatch` trigger, you can manually trigger it from the GitHub UI:

### Step-by-Step Instructions

#### Step 1: Go to Actions Tab
1. Navigate to: https://github.com/long123king/dk/actions
2. Click on "Release" workflow in the left sidebar

#### Step 2: Trigger Manually
1. Click "Run workflow" button (top right)
2. Select your branch from the dropdown
3. Click "Run workflow" button

#### Step 3: Monitor and Download
Follow the same steps as Method 1 (Steps 4-5)

---

## Comparison of Methods

| Method | Pros | Cons | Best For |
|--------|------|------|----------|
| **Tag on Branch** | Tests complete release process, creates actual release | Creates a release (needs cleanup) | Full end-to-end testing |
| **Pull Request** | Automatic, no cleanup needed | Doesn't test release creation | Quick build validation |
| **Manual Dispatch** | Easy to trigger, no tag needed | Needs workflow modification | Repeated testing |

---

## Troubleshooting

### Build Fails
- Check the workflow logs in the Actions tab
- Look for MSBuild errors or missing dependencies
- Ensure Windows SDK is available in the runner

### Release Not Created
- Verify the tag was pushed: `git ls-remote --tags origin`
- Check that the tag starts with 'v'
- Review the release workflow logs

### Artifacts Missing
- Verify the build completed successfully
- Check the output paths in workflow logs
- Ensure x64/Release directory was created

---

## Important Notes

1. **Tags work on any branch** - You don't need to merge to main first
2. **Use test version numbers** - Like `v0.0.1-test` to avoid confusion
3. **Clean up test releases** - Delete test releases after verification
4. **PR builds don't create releases** - They only validate the build works
5. **First run may take longer** - GitHub needs to set up the environment

---

## Next Steps After Testing

Once you've verified the release process works:

1. Delete any test releases and tags
2. Merge the PR to main/master
3. Create your first official release:
   ```bash
   git checkout main
   git pull
   git tag -a v1.0.0 -m "Initial release"
   git push origin v1.0.0
   ```

---

## Questions?

If you encounter issues:
1. Check the GitHub Actions logs
2. Review the workflow YAML files
3. See `RELEASE.md` for more details
