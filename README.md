# splitring

`splitring` lets you merge OS X Keychains by importing all the items
in one or more specified keychains into another keychain.

You can use it to get back to a single login keychain if you've
migrated to a new computer and are tired of typing your password to
unlock your old keychains.

__The bad news__ is that you'll have to click "Allow" in the keychain
authorization dialog once for every password that is copied. This is a
result of the way OS X limits access to the keychain--so much so that
even Keychain Access, a system utility, does it if you try to export
all of your passwords at once. The good news is that you'll never be
prompted randomly, at some inopportune time, to type an old keychain's
password.

The utility currently only imports passwords, as copying certificates
increases the risk of breaking a system service (I'm looking at you,
iCloud.) If you want to import certificates, keys, and identities,
grep the source for `kSecClassCertificate` and uncomment to your
heart's desire.

# Usage

    usage: splitring [-v | --verbose] [--version] [-d | --dry-run]
           [--to-keychain=<path>] keychain-file ...
    
    --verbose             Explain what's happening.
    --version             Print the version of the program
    --dry-run             Don't actually import any items,
                            just show what would be done.
    --to-keychain=<path>  Import all items to the keychain
                            located at <path>. By default,
                            this is the current default
                            keychain, which is usually the
                            login keychain.

# Example

    # Import all the passwords in old.keychain into the login keychain.
    $ ./splitring -v old.keychain
    Unlocking keychain at path: /Users/phil/old.keychain
    Importing 4 items to keychain at path: /Users/phil/Library/Keychains/login.keychain
    Copying item named 'AppleID' (1 of 4)... The specified item already exists in the keychain.
    Copying item named 'Safari Forms AutoFill' (2 of 4)... copied
    Copying item named 'skype' (3 of 4)... copied
    Copying item named 'photosmart' (4 of 4)... copied

# Building

To build `splitring`, you can use Xcode (open splitring.xcodeproj),
the `xcodebuild` tool, or `make`:

    $ make
    $ ./splitring

If you don't `codesign` the binary, you'll see an extra-scary message from OS X:

> "The authenticity of "splitring" cannot be verified. Do you want to
>  allow access to this item?"

This doesn't affect operation negatively.

To codesign, obtain or generate a certificate by following
[these instructions](https://developer.apple.com/library/mac/documentation/Security/Conceptual/CodeSigningGuide/Procedures/Procedures.html#//apple_ref/doc/uid/TP40005929-CH4-SW1)
and then run:

    $ codesign -s 'Name of Your Identity' splitring
