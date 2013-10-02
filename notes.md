Access ACLs:

    SecAccessRef access = NULL;
    OSStatus status = SecKeychainItemCopyAccess(item, &access);
    SRHandleError(status, true);

    CFArrayRef aclList = NULL;
    status = SecAccessCopyACLList(access, &aclList);
    SRHandleError(status, true);

    for (int j = 0; j < CFArrayGetCount(aclList); j++) {
      SecACLRef acl = (SecACLRef)CFArrayGetValueAtIndex(aclList, j);
      CFArrayRef applicationList = NULL;
      CFStringRef description = NULL;
      SecKeychainPromptSelector prompt = 0;
      status = SecACLCopyContents(acl, &applicationList, &description, &prompt);
      SRHandleError(status, true);

      printf("  %s\n", SRCFStringCopyUTF8String(description));
      CFShow(applicationList);

      if (description) { CFRelease(description); }
      if (applicationList) { CFRelease(applicationList); }
    }

    CFRelease(aclList);
    CFRelease(access);
