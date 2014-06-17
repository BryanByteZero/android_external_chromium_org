// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TOOLS_GN_TARGET_GENERATOR_H_
#define TOOLS_GN_TARGET_GENERATOR_H_

#include <string>
#include <vector>

#include "base/basictypes.h"
#include "base/memory/scoped_ptr.h"
#include "tools/gn/source_dir.h"
#include "tools/gn/target.h"

class BuildSettings;
class Err;
class FunctionCallNode;
class Location;
class Scope;
class Value;

// Fills the variables in a Target object from a Scope (the result of a script
// execution). Target-type-specific derivations of this class will be used
// for each different type of function call. This class implements the common
// behavior.
class TargetGenerator {
 public:
  TargetGenerator(Target* target,
                  Scope* scope,
                  const FunctionCallNode* function_call,
                  Err* err);
  ~TargetGenerator();

  void Run();

  // The function call is the parse tree node that invoked the target.
  // err() will be set on failure.
  static void GenerateTarget(Scope* scope,
                             const FunctionCallNode* function_call,
                             const std::vector<Value>& args,
                             const std::string& output_type,
                             Err* err);

 protected:
  // Derived classes implement this to do type-specific generation.
  virtual void DoRun() = 0;

  const BuildSettings* GetBuildSettings() const;

  void FillSources();
  void FillPublic();
  void FillInputs();
  void FillConfigs();
  void FillOutputs();

  Target* target_;
  Scope* scope_;
  const FunctionCallNode* function_call_;
  Err* err_;

 private:
  void FillDependentConfigs();  // Includes all types of dependent configs.
  void FillData();
  void FillDependencies();  // Includes data dependencies.

  // Reads configs/deps from the given var name, and uses the given setting on
  // the target to save them.
  void FillGenericConfigs(const char* var_name, LabelConfigVector* dest);
  void FillGenericDeps(const char* var_name, LabelTargetVector* dest);

  void FillForwardDependentConfigs();

  DISALLOW_COPY_AND_ASSIGN(TargetGenerator);
};

#endif  // TOOLS_GN_TARGET_GENERATOR_H_
