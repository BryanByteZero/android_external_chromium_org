// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

function afterCommit()
{
    try {
        debug("Accessing a committed transaction should throw");
        var store = transaction.objectStore('storeName');
    } catch (e) {
        exc = e;
        shouldBe('exc.code', 'DOMException.INVALID_STATE_ERR');
    }
    done();
}

function nonExistingKey()
{
    shouldBe("event.target.result", "undefined");
    transaction.oncomplete = afterCommit;
}

function gotValue()
{
    value = event.target.result;
    shouldBeEqualToString('value', 'myValue');
}

function startTransaction()
{
    debug("Using get in a transaction");
    transaction = db.transaction('storeName');
    //transaction.onabort = unexpectedErrorCallback;
    store = transaction.objectStore('storeName');
    shouldBeEqualToString("store.name", "storeName");
    request = store.get('myKey');
    request.onsuccess = gotValue;
    request.onerror = unexpectedErrorCallback;

    var emptyRequest = store.get('nonExistingKey');
    emptyRequest.onsuccess = nonExistingKey;
    emptyRequest.onerror = unexpectedErrorCallback;
}

function populateObjectStore()
{
    deleteAllObjectStores(db);
    window.objectStore = db.createObjectStore('storeName');
    var request = objectStore.add('myValue', 'myKey');
    request.onerror = unexpectedErrorCallback;
    event.target.result.oncomplete = startTransaction;
}

function setVersion()
{
    window.db = event.target.result;
    var request = db.setVersion('new version');
    request.onsuccess = populateObjectStore;
    request.onerror = unexpectedErrorCallback;
}

function test()
{
    request = webkitIndexedDB.open('name');
    request.onsuccess = setVersion;
    request.onerror = unexpectedErrorCallback;
}
