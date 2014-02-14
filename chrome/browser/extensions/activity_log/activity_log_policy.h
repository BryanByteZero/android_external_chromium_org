// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_ACTIVITY_LOG_ACTIVITY_LOG_POLICY_H_
#define CHROME_BROWSER_EXTENSIONS_ACTIVITY_LOG_ACTIVITY_LOG_POLICY_H_

#include <map>
#include <set>
#include <string>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/memory/scoped_ptr.h"
#include "base/values.h"
#include "chrome/browser/extensions/activity_log/activity_actions.h"
#include "chrome/browser/extensions/activity_log/activity_database.h"
#include "chrome/common/extensions/api/activity_log_private.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

class Profile;
class GURL;

namespace base {
class FilePath;
}

namespace extensions {

class Extension;

// An abstract class for processing and summarizing activity log data.
// Subclasses will generally store data in an SQLite database (the
// ActivityLogDatabasePolicy subclass includes some helper methods to assist
// with this case), but this is not absolutely required.
//
// Implementations should support:
// (1) Receiving Actions to process, and summarizing, compression, and storing
//     these as appropriate.
// (2) Reading Actions back from storage.
// (3) Cleaning of URLs
//
// Implementations based on a database should likely implement
// ActivityDatabase::Delegate, which provides hooks on database events and
// allows the database to periodically request that actions (which the policy
// is responsible for queueing) be flushed to storage.
//
// Since every policy implementation might summarize data differently, the
// database implementation is policy-specific and therefore completely
// encapsulated in the policy class.  All the member functions can be called
// on the UI thread.
class ActivityLogPolicy {
 public:
  enum PolicyType {
    POLICY_FULLSTREAM,
    POLICY_COUNTS,
    POLICY_INVALID,
  };

  // Parameters are the profile and the thread that will be used to execute
  // the callback when ReadData is called.
  // TODO(felt,dbabic)  Since only ReadData uses thread_id, it would be
  // cleaner to pass thread_id as a param of ReadData directly.
  explicit ActivityLogPolicy(Profile* profile);

  // Instead of a public destructor, ActivityLogPolicy objects have a Close()
  // method which will cause the object to be deleted (but may do so on another
  // thread or in a deferred fashion).
  virtual void Close() = 0;

  // Updates the internal state of the model summarizing actions and possibly
  // writes to the database.  Implements the default policy storing internal
  // state to memory every 5 min.
  virtual void ProcessAction(scoped_refptr<Action> action) = 0;

  // For unit testing only.
  void SetClockForTesting(scoped_ptr<base::Clock> clock);

  // A collection of methods that are useful for implementing policies.  These
  // are all static methods; the ActivityLogPolicy::Util class cannot be
  // instantiated.  This is nested within ActivityLogPolicy to make calling
  // these methods more convenient from within a policy, but they are public.
  class Util {
   public:
    // A collection of API calls, used to specify whitelists for argument
    // filtering.
    typedef std::set<std::pair<Action::ActionType, std::string> > ApiSet;

    // Serialize a Value as a JSON string.  Returns an empty string if value is
    // null.
    static std::string Serialize(const base::Value* value);

    // Removes potentially privacy-sensitive data that should not be logged.
    // This should generally be called on an Action before logging, unless
    // debugging flags are enabled.  Modifies the Action object in place; if
    // the action might be shared with other users, it is up to the caller to
    // call ->Clone() first.
    static void StripPrivacySensitiveFields(scoped_refptr<Action> action);

    // Strip arguments from most API actions, preserving actions only for a
    // whitelisted set.  Modifies the Action object in-place.
    static void StripArguments(const ApiSet& api_whitelist,
                               scoped_refptr<Action> action);

    // Given a base day (timestamp at local midnight), computes the timestamp
    // at midnight the given number of days before or after.
    static base::Time AddDays(const base::Time& base_date, int days);

    // Compute the time bounds that should be used for a database query to
    // cover a time range days_ago days in the past, relative to the specified
    // time.
    static void ComputeDatabaseTimeBounds(const base::Time& now,
                                          int days_ago,
                                          int64* early_bound,
                                          int64* late_bound);

    // Deletes obsolete database tables from an activity log database.  This
    // can be used in InitDatabase() methods of ActivityLogDatabasePolicy
    // subclasses to clean up data from old versions of the activity logging
    // code.  Returns true on success, false on database error.
    static bool DropObsoleteTables(sql::Connection* db);

   private:
    DISALLOW_IMPLICIT_CONSTRUCTORS(Util);
  };

 protected:
  // An ActivityLogPolicy is not directly destroyed.  Instead, call Close()
  // which will cause the object to be deleted when it is safe.
  virtual ~ActivityLogPolicy();

  // Returns Time::Now() unless a mock clock has been installed with
  // SetClockForTesting, in which case the time according to that clock is used
  // instead.
  base::Time Now() const;

 private:
  // Support for a mock clock for testing purposes.  This is used by ReadData
  // to determine the date for "today" when when interpreting date ranges to
  // fetch.  This has no effect on batching of writes to the database.
  scoped_ptr<base::Clock> testing_clock_;

  DISALLOW_COPY_AND_ASSIGN(ActivityLogPolicy);
};

// A subclass of ActivityLogPolicy which is designed for policies that use
// database storage; it contains several useful helper methods.
class ActivityLogDatabasePolicy : public ActivityLogPolicy,
                                  public ActivityDatabase::Delegate {
 public:
  ActivityLogDatabasePolicy(Profile* profile,
                            const base::FilePath& database_name);

  // Initializes an activity log policy database. This needs to be called after
  // constructing ActivityLogDatabasePolicy.
  void Init();

  // Requests that in-memory state be written to the database.  This method can
  // be called from any thread, but the database writes happen asynchronously
  // on the database thread.
  virtual void Flush();

  // Gets all actions that match the specified fields. URLs are treated like
  // prefixes; other fields are exact matches. Empty strings are not matched to
  // anything. For the date: 0 = today, 1 = yesterday, etc.; if the data is
  // negative, it will be treated as missing.
  virtual void ReadFilteredData(
      const std::string& extension_id,
      const Action::ActionType type,
      const std::string& api_name,
      const std::string& page_url,
      const std::string& arg_url,
      const int days_ago,
      const base::Callback
         <void(scoped_ptr<Action::ActionVector>)>& callback) = 0;

  // Remove actions (rows) which IDs are in the action_ids array.
  virtual void RemoveActions(const std::vector<int64>& action_ids) = 0;

  // Clean the relevant URL data. The cleaning may need to be different for
  // different policies. If restrict_urls is empty then all URLs are removed.
  virtual void RemoveURLs(const std::vector<GURL>& restrict_urls) = 0;

  // Remove all rows relating to a given extension.
  virtual void RemoveExtensionData(const std::string& extension_id) = 0;

  // Deletes everything in the database.
  virtual void DeleteDatabase() = 0;

 protected:
  // The Schedule methods dispatch the calls to the database on a
  // separate thread.
  template<typename DatabaseType, typename DatabaseFunc>
  void ScheduleAndForget(DatabaseType db, DatabaseFunc func) {
    content::BrowserThread::PostTask(
        content::BrowserThread::DB,
        FROM_HERE,
        base::Bind(func, base::Unretained(db)));
  }

  template<typename DatabaseType, typename DatabaseFunc, typename ArgA>
  void ScheduleAndForget(DatabaseType db, DatabaseFunc func, ArgA a) {
    content::BrowserThread::PostTask(
        content::BrowserThread::DB,
        FROM_HERE,
        base::Bind(func, base::Unretained(db), a));
  }

  template<typename DatabaseType, typename DatabaseFunc,
      typename ArgA, typename ArgB>
  void ScheduleAndForget(DatabaseType db, DatabaseFunc func, ArgA a, ArgB b) {
    content::BrowserThread::PostTask(
        content::BrowserThread::DB,
        FROM_HERE,
        base::Bind(func, base::Unretained(db), a, b));
  }

  // Access to the underlying ActivityDatabase.
  ActivityDatabase* activity_database() const { return db_; }

  // Access to the SQL connection in the ActivityDatabase.  This should only be
  // called from the database thread.  May return NULL if the database is not
  // valid.
  sql::Connection* GetDatabaseConnection() const;

 private:
  // See the comments for the ActivityDatabase class for a discussion of how
  // database cleanup runs.
  ActivityDatabase* db_;
  base::FilePath database_path_;
};

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_ACTIVITY_LOG_ACTIVITY_LOG_POLICY_H_
