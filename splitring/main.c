//
//  main.c
//  splitring
//
//  Created by Phil Calvin (Eleos) on 9/29/13.
//  Copyright (c) 2013 Phil Calvin. All rights reserved.
//

#include <CoreFoundation/CoreFoundation.h>
#import <Security/Security.h>

void SRHandleError(OSStatus status, int requireSuccess);
char * SRCFStringCopyUTF8String(CFStringRef aString);
void SRCopyMatchingItemsToKeychain(CFDictionaryRef query, SecKeychainRef keychain);
void SRListMatchingItems(CFDictionaryRef query);
void SRCopyKeychainItemToKeychain(SecKeychainItemRef item, SecKeychainRef keychain);

int main(int argc, const char * argv[])
{
  char *path = "/Users/phil/Nebula/Nebula Legacy.keychain";

  SecKeychainRef keychain = NULL;
  OSStatus status = SecKeychainOpen(path, &keychain);
  SRHandleError(status, true);

  // Unlock it; might not need to do this
  status = SecKeychainUnlock(keychain, 0, NULL, FALSE);
  SRHandleError(status, true);

  CFMutableArrayRef keychains = CFArrayCreateMutable(NULL, 0, &kCFTypeArrayCallBacks);
  CFArrayAppendValue(keychains, keychain);

  // Open the default keychain and print its path
  SecKeychainRef defaultKeychain = NULL;
  status = SecKeychainCopyDefault(&defaultKeychain);
  SRHandleError(status, true);
  status = SecKeychainUnlock(defaultKeychain, 0, NULL, FALSE);
  SRHandleError(status, true);
  UInt32 bufferSize = 512;
  char defaultKeychainPath[512];
  SecKeychainGetPath(defaultKeychain, &bufferSize, defaultKeychainPath);
  printf("Default keychain path: %s", defaultKeychainPath);

  // Search for all items
  CFMutableDictionaryRef query = CFDictionaryCreateMutable(NULL, 0,
                                                           &kCFTypeDictionaryKeyCallBacks,
                                                           &kCFTypeDictionaryValueCallBacks);
  CFDictionarySetValue(query, kSecMatchSearchList, keychains);
  CFDictionarySetValue(query, kSecClass, kSecClassInternetPassword);
  CFDictionarySetValue(query, kSecReturnRef, kCFBooleanTrue);
  CFDictionarySetValue(query, kSecReturnAttributes, kCFBooleanTrue);
  CFDictionarySetValue(query, kSecMatchLimit, kSecMatchLimitAll);

  SRCopyMatchingItemsToKeychain(query, defaultKeychain);

  CFRelease(query);
  CFRelease(defaultKeychain);
  CFRelease(keychains);
  CFRelease(keychain);
  return 0;
}

// LEAKS
// I DON'T EVEN CARE
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

void SRCopyMatchingItemsToKeychain(CFDictionaryRef query, SecKeychainRef keychain) {
  CFArrayRef items = NULL;
  OSStatus status = SecItemCopyMatching(query, (CFTypeRef *)&items);
  SRHandleError(status, true);

  // Copy these items into the provided keychain
  for (int i = 0; i < CFArrayGetCount(items); i++) {
    CFDictionaryRef info = CFArrayGetValueAtIndex(items, i);
    CFStringRef label = CFDictionaryGetValue(info, kSecAttrLabel);

    char *cLabel = SRCFStringCopyUTF8String(label);

    printf("Copying item named '%s'… ", cLabel);
    SecKeychainItemRef item = (SecKeychainItemRef)CFDictionaryGetValue(info, kSecValueRef);
    SRCopyKeychainItemToKeychain(item, keychain);
    printf("%li items to go.\n", CFArrayGetCount(items) - i - 1);
  }

  CFRelease(items);
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
      printf("done!\n");
      CFRelease(newItem);
    } else {
      printf("failed.\n");
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
      exit(15);
    }
  }
}
