// Copyright (c) 2010 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_AUTOFILL_AUTOFILL_DIALOG_CONTROLLER_MAC_
#define CHROME_BROWSER_AUTOFILL_AUTOFILL_DIALOG_CONTROLLER_MAC_

#import <Cocoa/Cocoa.h>
#include <vector>
#include "base/scoped_nsobject.h"
#include "base/scoped_ptr.h"
#include "chrome/browser/autofill/autofill_dialog.h"
#include "chrome/browser/autofill/autofill_profile.h"
#include "chrome/browser/autofill/credit_card.h"

namespace AutoFillDialogControllerInternal {
class PersonalDataManagerObserver;
}  // AutoFillDialogControllerInternal

@class AutoFillAddressSheetController;
@class AutoFillCreditCardSheetController;
@class AutoFillTableView;
class Profile;
@class WindowSizeAutosaver;

// A window controller for managing the autofill options dialog.
// Application modally presents a dialog allowing the user to store
// personal address and credit card information.
@interface AutoFillDialogController : NSWindowController {
 @private
  // Outlet to the main NSTableView object listing both addresses and credit
  // cards with section headers for both.
  IBOutlet AutoFillTableView* tableView_;

  // This observer is passed in by the caller of the dialog.  When the dialog
  // is dismissed |observer_| is called with new values for the addresses and
  // credit cards.
  // Weak, not retained.
  AutoFillDialogObserver* observer_;

  // Reference to input parameter.
  // Weak, not retained.
  Profile* profile_;

  // Reference to input parameter.
  // Weak, not retained.
  AutoFillProfile* importedProfile_;

  // Reference to input parameter.
  // Weak, not retained.
  CreditCard* importedCreditCard_;

  // Working list of input profiles.
  std::vector<AutoFillProfile> profiles_;

  // Working list of input credit cards.
  std::vector<CreditCard> creditCards_;

  // State of checkbox for enabling Mac Address Book integration.
  BOOL auxiliaryEnabled_;

  // State for |itemIsSelected| property used in bindings for "Edit..." and
  // "Remove" buttons.
  BOOL itemIsSelected_;

  // Utility object to save and restore dialog position.
  scoped_nsobject<WindowSizeAutosaver> sizeSaver_;

  // Transient reference to address "Add" / "Edit" sheet for address
  // information.
  scoped_nsobject<AutoFillAddressSheetController> addressSheetController;

  // Transient reference to address "Add" / "Edit" sheet for credit card
  // information.
  scoped_nsobject<AutoFillCreditCardSheetController> creditCardSheetController;

  // Manages PersonalDataManager loading.
  scoped_ptr<AutoFillDialogControllerInternal::PersonalDataManagerObserver>
      personalDataManagerObserver_;
}

// Property representing state of Address Book "me" card usage.  Checkbox is
// bound to this in nib.
@property (nonatomic) BOOL auxiliaryEnabled;

// Property representing selection state in |tableView_|.  Enabled state of
// edit and delete buttons are bound to this property.
@property (nonatomic) BOOL itemIsSelected;

// Main interface for displaying an application modal AutoFill dialog on screen.
// This class method creates a new |AutoFillDialogController| and runs it as a
// modal dialog.  The controller autoreleases itself when the dialog is closed.
// |observer| can be NULL, but if it is, then no notification is sent during
// call to |save|.  If |observer| is non-NULL then its |OnAutoFillDialogApply|
// method is invoked during |save| with the new address and credit card
// information.
// |profile| must be non-NULL.
// AutoFill profile and credit card data is initialized from the
// |PersonalDataManager| that is associated with the input |profile|.
// If |importedProfile| or |importedCreditCard| parameters are supplied then
// the |PersonalDataManager| data is ignored.  Both may be NULL.
+ (void)showAutoFillDialogWithObserver:(AutoFillDialogObserver*)observer
                     profile:(Profile*)profile
             importedProfile:(AutoFillProfile*)importedProfile
          importedCreditCard:(CreditCard*)importedCreditCard;

// IBActions for the dialog buttons.
- (IBAction)save:(id)sender;
- (IBAction)cancel:(id)sender;

// IBActions for adding new items.
- (IBAction)addNewAddress:(id)sender;
- (IBAction)addNewCreditCard:(id)sender;

// IBAction for deleting an item.  |sender| is expected to be the "Remove"
// button.  The deletion acts on the selected item in either the address or
// credit card list.
- (IBAction)deleteSelection:(id)sender;

// IBActions for editing an item.  |sender| is expected to be the "Edit..."
// button.  The editing acts on the selected item in either the address or
// credit card list.
- (IBAction)editSelection:(id)sender;

// NSTableView data source methods.
- (id)tableView:(NSTableView *)tableView
    objectValueForTableColumn:(NSTableColumn *)tableColumn
                          row:(NSInteger)rowIndex;

- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView;

// Returns an array of labels representing the addresses in the |profiles_|.
- (NSArray*)addressLabels;

@end

// Interface exposed for unit testing.
@interface AutoFillDialogController (ExposedForUnitTests)
// Returns an instance of AutoFillDialogController.  See |-initWithObserver|
// for details about arguments.
// Note: controller is autoreleased when |-closeDialog| is called.
+ (AutoFillDialogController*)controllerWithObserver:
      (AutoFillDialogObserver*)observer
               profile:(Profile*)profile
       importedProfile:(AutoFillProfile*)importedProfile
    importedCreditCard:(CreditCard*)importedCreditCard;

- (id)initWithObserver:(AutoFillDialogObserver*)observer
               profile:(Profile*)profile
       importedProfile:(AutoFillProfile*)importedProfile
    importedCreditCard:(CreditCard*)importedCreditCard;
- (void)closeDialog;
- (AutoFillAddressSheetController*)addressSheetController;
- (AutoFillCreditCardSheetController*)creditCardSheetController;
- (void)selectAddressAtIndex:(size_t)i;
- (void)selectCreditCardAtIndex:(size_t)i;
@end

#endif  // CHROME_BROWSER_AUTOFILL_AUTOFILL_DIALOG_CONTROLLER_MAC_
