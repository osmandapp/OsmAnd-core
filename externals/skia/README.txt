How to update SKIA to new release:

1. Check what is the latest release of Google Chrome. E.g. at this moment it's 37.0.2062.103
2. Options:
2.1. If available, check https://chromium.googlesource.com/chromium/src/+/37.0.2062.103
2.2. If above option didn't work, go to https://src.chromium.org/viewvc/chrome/releases/ and find that release
3. Get the DEPS file
4. Find lines that look like:
  'src/third_party/skia':
    (Var("git.chromium.org")) + '/skia.git@...',
5. There will be the revision of SKIA included in the release. E.g. for 37.0.2062.103 it's 9112a5fed24da8442a5d263c634adbbf6b57862c
6. Check if available at https://github.com/google/skia/tree/9112a5fed24da8442a5d263c634adbbf6b57862c and rebase to it, if available
