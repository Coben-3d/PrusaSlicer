/* Test-only isolation: never read, create, modify or delete a macOS keychain item. */
#include <Security/Security.h>
#include <stdio.h>
#define BLOCKED() fputs("TEST_KEYCHAIN_BLOCKED\n", stderr)
static OSStatus deny_find(CFTypeRef kc, UInt32 sl, const char* service, UInt32 al,
                         const char* account, UInt32* length, void** data, SecKeychainItemRef* item) {
    BLOCKED(); if (length) *length = 0; if (data) *data = NULL; if (item) *item = NULL;
    return errSecItemNotFound;
}
static OSStatus deny_add(SecKeychainRef kc, UInt32 sl, const char* service, UInt32 al,
                        const char* account, UInt32 length, const void* data, SecKeychainItemRef* item) {
    BLOCKED(); if (item) *item = NULL; return errSecAuthFailed;
}
static OSStatus deny_delete(SecKeychainItemRef item) { BLOCKED(); return errSecAuthFailed; }
static OSStatus deny_copy(SecKeychainItemRef item, const SecKeychainAttributeInfo* info,
                         SecItemClass* cls, SecKeychainAttributeList** attrs, UInt32* length, void** data) {
    BLOCKED(); if (attrs) *attrs = NULL; if (length) *length = 0; if (data) *data = NULL;
    return errSecAuthFailed;
}
static OSStatus deny_item_copy(CFDictionaryRef query, CFTypeRef* result) {
    BLOCKED(); if (result) *result = NULL; return errSecItemNotFound;
}
static OSStatus deny_item_add(CFDictionaryRef attrs, CFTypeRef* result) {
    BLOCKED(); if (result) *result = NULL; return errSecAuthFailed;
}
static OSStatus deny_item_update(CFDictionaryRef query, CFDictionaryRef attrs) { BLOCKED(); return errSecAuthFailed; }
static OSStatus deny_item_delete(CFDictionaryRef query) { BLOCKED(); return errSecAuthFailed; }
#define INTERPOSE(replacement, original) \
    __attribute__((used, section("__DATA,__interpose"))) static const struct { \
        const void* replacement; const void* original; \
    } pair_##original = { (const void*)replacement, (const void*)original }
INTERPOSE(deny_find, SecKeychainFindGenericPassword);
INTERPOSE(deny_add, SecKeychainAddGenericPassword);
INTERPOSE(deny_delete, SecKeychainItemDelete);
INTERPOSE(deny_copy, SecKeychainItemCopyAttributesAndData);
INTERPOSE(deny_item_copy, SecItemCopyMatching);
INTERPOSE(deny_item_add, SecItemAdd);
INTERPOSE(deny_item_update, SecItemUpdate);
INTERPOSE(deny_item_delete, SecItemDelete);
