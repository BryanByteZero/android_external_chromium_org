// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.closure.compiler;

import com.google.javascript.jscomp.CodingConvention;
import com.google.javascript.jscomp.CodingConventions;
import com.google.javascript.jscomp.DiagnosticType;
import com.google.common.collect.ImmutableSet;
import com.google.common.collect.Sets;
import com.google.javascript.rhino.Node;
import com.google.javascript.rhino.jstype.FunctionType;
import com.google.javascript.rhino.jstype.ObjectType;

import java.util.Collection;
import java.util.List;
import java.util.Set;

public class ChromeCodingConvention extends CodingConventions.Proxy {

  private final Set<String> indirectlyDeclaredProperties;

  public ChromeCodingConvention() {
    this(CodingConventions.getDefault());
  }

  public ChromeCodingConvention(CodingConvention wrapped) {
    super(wrapped);

    Set<String> props = Sets.newHashSet("instance_", "getInstance");
    props.addAll(wrapped.getIndirectlyDeclaredProperties());
    indirectlyDeclaredProperties = ImmutableSet.copyOf(props);
  }

  @Override
  public String getSingletonGetterClassName(Node callNode) {
    Node callArg = callNode.getFirstChild();

    if (!(callArg.matchesQualifiedName("cr.addSingletonGetter")) ||
        callNode.getChildCount() != 2) {
      return super.getSingletonGetterClassName(callNode);
    }

    return callArg.getNext().getQualifiedName();
  }

  @Override
  public void applySingletonGetter(FunctionType functionType,
      FunctionType getterType, ObjectType objectType) {
    super.applySingletonGetter(functionType, getterType, objectType);
    functionType.defineDeclaredProperty("getInstance", getterType,
        functionType.getSource());
    functionType.defineDeclaredProperty("instance_", objectType,
        functionType.getSource());
  }

  @Override
  public Collection<String> getIndirectlyDeclaredProperties() {
    return indirectlyDeclaredProperties;
  }

}
