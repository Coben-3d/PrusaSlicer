// Native macOS parent for the unchanged PrusaSlicer executable.
// SPDX-License-Identifier: AGPL-3.0-or-later
#import <Cocoa/Cocoa.h>

#ifndef PRUSA_LAUNCHER_IDENTIFIER
#error Define PRUSA_LAUNCHER_IDENTIFIER for each application variant.
#endif

@interface FilamentsLauncher : NSObject <NSApplicationDelegate>
@property(nonatomic, strong) NSTask *slicer;
@end

@implementation FilamentsLauncher
- (void)applicationDidFinishLaunching:(NSNotification *)notification
{
    (void)notification;
    NSBundle *bundle = NSBundle.mainBundle;
    NSString *settings = [bundle objectForInfoDictionaryKey:@"PrusaSlicerDataDirectory"];
    NSSet *allowed = [NSSet setWithArray:@[@"settings", @"settings-mk4", @"settings-indx"]];
    if (![bundle.bundleIdentifier isEqualToString:@PRUSA_LAUNCHER_IDENTIFIER] ||
        ![allowed containsObject:settings]) {
        [self failWithMessage:@"Le lanceur et sa configuration ne correspondent pas."];
        return;
    }

    NSURL *package = [bundle.bundleURL URLByDeletingLastPathComponent];
    NSURL *binary = [package URLByAppendingPathComponent:@"slicer/MacOS/PrusaSlicer"];
    NSURL *data = [package URLByAppendingPathComponent:settings isDirectory:YES];
    if (![NSFileManager.defaultManager isExecutableFileAtPath:binary.path]) {
        [self failWithMessage:@"L’exécutable PrusaSlicer est introuvable. Conservez le dossier extrait complet."];
        return;
    }

    NSMutableDictionary *environment = [NSProcessInfo.processInfo.environment mutableCopy];
    [environment removeObjectForKey:@"DYLD_INSERT_LIBRARIES"];
    self.slicer = [[NSTask alloc] init];
    self.slicer.executableURL = binary;
    self.slicer.arguments = @[@"--datadir", data.path, @"--single-instance"];
    self.slicer.environment = environment;
    self.slicer.standardOutput = NSFileHandle.fileHandleWithNullDevice;
    self.slicer.standardError = NSFileHandle.fileHandleWithNullDevice;
    self.slicer.terminationHandler = ^(NSTask *task) {
        (void)task;
        dispatch_async(dispatch_get_main_queue(), ^{
            [NSApp terminate:nil];
        });
    };
    NSError *error = nil;
    if (![self.slicer launchAndReturnError:&error]) {
        self.slicer = nil;
        [self failWithMessage:@"PrusaSlicer n’a pas pu être lancé. Vérifiez l’accès au dossier et l’autorisation d’ouverture macOS."];
    }
}

- (void)failWithMessage:(NSString *)message
{
    NSAlert *alert = [[NSAlert alloc] init];
    alert.messageText = @"PrusaSlicer Filaments";
    alert.informativeText = message;
    [alert runModal];
    [NSApp terminate:nil];
}

- (BOOL)applicationShouldHandleReopen:(NSApplication *)sender hasVisibleWindows:(BOOL)visible
{
    (void)sender;
    (void)visible;
    if (self.slicer.running) {
        NSRunningApplication *child = [NSRunningApplication runningApplicationWithProcessIdentifier:self.slicer.processIdentifier];
        [child activateWithOptions:0];
    }
    return YES;
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender
{
    (void)sender;
    // Never discard the user's unsaved Slicer project by killing its process.
    if (self.slicer.running) {
        [self applicationShouldHandleReopen:NSApp hasVisibleWindows:NO];
        return NSTerminateCancel;
    }
    return NSTerminateNow;
}
@end

int main(void)
{
    @autoreleasepool {
        NSApplication *app = NSApplication.sharedApplication;
        [app setActivationPolicy:NSApplicationActivationPolicyAccessory];
        // NSApplication does not retain its delegate. Keep it alive while run blocks.
        __attribute__((objc_precise_lifetime)) FilamentsLauncher *delegate = [[FilamentsLauncher alloc] init];
        app.delegate = delegate;
        [app run];
    }
    return 0;
}
