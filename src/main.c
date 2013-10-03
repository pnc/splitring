//
//  main.c
//  splitring
//
//  Created by Phil Calvin (Eleos) on 9/29/13.
//  Copyright (c) 2013 Phil Calvin. All rights reserved.
//

#include <CoreFoundation/CoreFoundation.h>
#import <Security/Security.h>
#import <getopt.h>
#import <sysexits.h>

void SRHandleError(OSStatus status, int requireSuccess);
char * SRCFStringCopyUTF8String(CFStringRef aString);
void SRListMatchingItems(CFDictionaryRef query);
CFArrayRef SRCopyItems(CFArrayRef keychains, CFArrayRef classes);
void SRCopyItemsToKeychain(CFArrayRef items, SecKeychainRef keychain, int verbose, int dryRun);
void SRCopyKeychainItemToKeychain(SecKeychainItemRef item, SecKeychainRef keychain);
SecKeychainRef SROpenKeychain(char* path);
void SRPrintUsage();

static CFStringRef kSRAttrClass = CFSTR("SRClass");

int main(int argc, char * const argv[]) {
  int verbose = 0;
  int dryRun = 0;
  char *toKeychainPath = NULL;

  struct option longopts[] = {
    { "dry-run",      no_argument,         &dryRun,     1   },
    { "to-keychain",  required_argument,   NULL,        'k' },
    { "verbose",      no_argument,         &verbose,    1   },
    { NULL,           0,                   NULL,        0   }
  };

  int ch;
  while ((ch = getopt_long(argc, argv, "dk:v", longopts, NULL)) != -1) {
    switch (ch) {
      case 'k':
        toKeychainPath = optarg;
        break;
      case 'd':
        dryRun = 1;
        break;
      case 'v':
        verbose = 1;
        break;
      default:
        SRPrintUsage();
        exit(EX_USAGE);
    }
  }
  argc -= optind;
  argv += optind;

  if (!argc) {
    SRPrintUsage();
    exit(EX_USAGE);
  }

  CFMutableArrayRef keychains = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);

  // Iterate the passed keychain paths, opening each one
  for (int i = 0; i < argc; i++) {
    char* path = argv[i];
    SecKeychainRef keychain = SROpenKeychain(path);
    CFArrayAppendValue(keychains, keychain);
    CFRelease(keychain);
  }

  // Unlock each of the keychains
  for (int i = 0; i < CFArrayGetCount(keychains); i++) {
    SecKeychainRef keychain = (SecKeychainRef)CFArrayGetValueAtIndex(keychains, i);
    // Unlock it; might not need to do this
    if (verbose) {
      UInt32 bufferSize = 4096;
      char keychainPath[bufferSize];
      SecKeychainGetPath(keychain, &bufferSize, keychainPath);
      fprintf(stderr, "Unlocking keychain at path: %s\n", keychainPath);
    }
    OSStatus status = SecKeychainUnlock(keychain, 0, NULL, FALSE);
    SRHandleError(status, true);
  }

  SecKeychainRef targetKeychain = NULL;
  if (toKeychainPath) {
    // Import to the specified keychain
    targetKeychain = SROpenKeychain(toKeychainPath);
  } else {
    // Import to the default keychain if one wasn't specified
    OSStatus status = SecKeychainCopyDefault(&targetKeychain);
    SRHandleError(status, true);
    status = SecKeychainUnlock(targetKeychain, 0, NULL, FALSE);
    SRHandleError(status, true);
  }

  // Search for all items
  CFMutableArrayRef classes = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
  CFArrayAppendValue(classes, kSecClassGenericPassword);
  CFArrayAppendValue(classes, kSecClassInternetPassword);
  //CFArrayAppendValue(classes, kSecClassCertificate);
  //CFArrayAppendValue(classes, kSecClassIdentity);
  //CFArrayAppendValue(classes, kSecClassKey);

  CFArrayRef items = SRCopyItems(keychains, classes);

  if (verbose) {
    UInt32 bufferSize = 4096;
    char defaultKeychainPath[bufferSize];
    SecKeychainGetPath(targetKeychain, &bufferSize, defaultKeychainPath);
    fprintf(stderr, "Importing %li items to keychain at path: %s\n",
            CFArrayGetCount(items),
            defaultKeychainPath);
  }

  SRCopyItemsToKeychain(items, targetKeychain, verbose, dryRun);

  CFRelease(items);
  CFRelease(targetKeychain);
  CFRelease(keychains);
  return 0;
}

void SRPrintUsage() {
  fprintf(stderr, "usage: splitring [-v | --verbose] [-d | --dry-run] [--to-keychain=<path>] keychain-file ...\n");
}

SecKeychainRef SROpenKeychain(char* path) {
  SecKeychainRef keychain = NULL;
  OSStatus status = SecKeychainOpen(path, &keychain);
  SecKeychainStatus keychainStatus = 0;
  status = SecKeychainGetStatus(keychain, &keychainStatus);
  if (status == 0) {
  } else {
    fprintf(stderr, "Unable to open keychain at path %s: ", path);
    SRHandleError(status, false);
    exit(EX_IOERR);
  }
  return keychain;
}

CFArrayRef SRCopyItems(CFArrayRef keychains, CFArrayRef classes) {
  CFMutableArrayRef allItems = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);

  CFMutableDictionaryRef query = CFDictionaryCreateMutable(NULL, 0,
                                                           &kCFTypeDictionaryKeyCallBacks,
                                                           &kCFTypeDictionaryValueCallBacks);
  CFDictionarySetValue(query, kSecMatchSearchList, keychains);
  CFDictionarySetValue(query, kSecReturnRef, kCFBooleanTrue);
  CFDictionarySetValue(query, kSecReturnAttributes, kCFBooleanTrue);
  CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitAll);

  for (int i = 0; i < CFArrayGetCount(classes); i++) {
    CFTypeRef class = CFArrayGetValueAtIndex(classes, i);
    CFDictionarySetValue(query, kSecClass, class);

    CFArrayRef items = NULL;
    OSStatus status = SecItemCopyMatching(query, (CFTypeRef *)&items);
    SRHandleError(status, true);
    for (int j = 0; j < CFArrayGetCount(items); j++) {
      CFDictionaryRef properties = CFArrayGetValueAtIndex(items, j);
      CFMutableDictionaryRef newProperties = CFDictionaryCreateMutableCopy(NULL, 0, properties);
      CFDictionarySetValue(newProperties, kSRAttrClass, class);
      CFArrayAppendValue(allItems, newProperties);
      CFRelease(newProperties);
    }
    CFRelease(items);
  }
  CFRelease(query);

  return allItems;
}

char * SRCFStringCopyUTF8String(CFStringRef aString) {
  if (aString == NULL) {
    return NULL;
  }

  CFIndex length = CFStringGetLength(aString);
  CFIndex maxSize =
  CFStringGetMaximumSizeForEncoding(length,
                                    kCFStringEncodingUTF8);
  char *buffer = (char *)malloc(maxSize);
  if (CFStringGetCString(aString, buffer, maxSize,
                         kCFStringEncodingUTF8)) {
    return buffer;
  }
  return NULL;
}

void SRCopyItemsToKeychain(CFArrayRef items, SecKeychainRef keychain, int verbose, int dryRun) {
  // Copy these items into the provided keychain
  for (int i = 0; i < CFArrayGetCount(items); i++) {
    CFDictionaryRef info = CFArrayGetValueAtIndex(items, i);
    CFTypeRef class = CFDictionaryGetValue(info, kSRAttrClass);
    CFStringRef label = CFDictionaryGetValue(info, kSecAttrLabel);

    // Form a printable name for the item
    char *cLabel = SRCFStringCopyUTF8String(label);
    char *cClass = "(unknown)";
    if (CFStringCompare(class, kSecClassGenericPassword, 0) == 0) {
      cClass = "Generic Password";
    } else if (CFStringCompare(class, kSecClassInternetPassword, 0) == 0) {
      cClass = "Internet Password";
    } else if (CFStringCompare(class, kSecClassCertificate, 0) == 0) {
      cClass = "Certificate";
    } else if (CFStringCompare(class, kSecClassIdentity, 0) == 0) {
      cClass = "Identity";
    } else if (CFStringCompare(class, kSecClassKey, 0) == 0) {
      cClass = "Key";
    }

    fprintf(stderr, "Copying item named '%s' (%i of %li)... ", cLabel, i + 1, CFArrayGetCount(items));
    free(cLabel);

    SecKeychainItemRef item = (SecKeychainItemRef)CFDictionaryGetValue(info, kSecValueRef);
    if (!dryRun) {
      SRCopyKeychainItemToKeychain(item, keychain);
    } else {
      fprintf(stderr, "\n");
    }
  }
}

void SRCopyKeychainItemToKeychain(SecKeychainItemRef item, SecKeychainRef keychain) {
  SecAccessRef accessPolicy = NULL;
  OSStatus status = SecKeychainItemCopyAccess(item, &accessPolicy);
  SRHandleError(status, false);
  if (0 == status) {
    SecKeychainItemRef newItem = NULL;
    status = SecKeychainItemCreateCopy(item, keychain, accessPolicy, &newItem);
    SRHandleError(status, false);
    if (0 == status) {
      fprintf(stderr, "copied.\n");
      CFRelease(newItem);
    }
    CFRelease(accessPolicy);
  }
}

void SRHandleError(OSStatus status, int requireSuccess) {
  if (status) {
    CFStringRef errorMessage = SecCopyErrorMessageString(status, NULL);
    CFShow(errorMessage);
    CFRelease(errorMessage);
    if (requireSuccess) {
      exit(EX_NOPERM);
    }
  }
}
