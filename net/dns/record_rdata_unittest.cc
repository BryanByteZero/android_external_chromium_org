// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/memory/scoped_ptr.h"
#include "net/base/net_util.h"
#include "net/dns/dns_response.h"
#include "net/dns/record_rdata.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace net {

TEST(RecordRdataTest, ParseSrvRecord) {
  scoped_ptr<SrvRecordRdata> record1_obj;
  scoped_ptr<SrvRecordRdata> record2_obj;

  // These are just the rdata portions of the DNS records, rather than complete
  // records, but it works well enough for this test.

  const char record[] = {
    '\x00', '\x01',
    '\x00', '\x02',
    '\x00', '\x50',
    '\x03', 'w', 'w', 'w',
    '\x06', 'g', 'o', 'o', 'g', 'l', 'e',
    '\x03', 'c', 'o', 'm',
    '\x00',
    '\x01', '\x01',
    '\x01', '\x02',
    '\x01', '\x03',
    '\x04', 'w', 'w', 'w', '2',
    '\xc0', '\x0a',  // Pointer to "google.com"
  };

  DnsRecordParser parser(record, sizeof(record), 0);
  const unsigned first_record_len = 22;
  base::StringPiece record1_strpiece(record, first_record_len);
  base::StringPiece record2_strpiece(
      record + first_record_len, sizeof(record) - first_record_len);

  record1_obj = SrvRecordRdata::Create(record1_strpiece, parser);
  ASSERT_TRUE(record1_obj != NULL);
  ASSERT_EQ(1, record1_obj->priority());
  ASSERT_EQ(2, record1_obj->weight());
  ASSERT_EQ(80, record1_obj->port());

  ASSERT_EQ("www.google.com", record1_obj->target());

  record2_obj = SrvRecordRdata::Create(record2_strpiece, parser);
  ASSERT_TRUE(record2_obj != NULL);
  ASSERT_EQ(257, record2_obj->priority());
  ASSERT_EQ(258, record2_obj->weight());
  ASSERT_EQ(259, record2_obj->port());

  ASSERT_EQ("www2.google.com", record2_obj->target());

  ASSERT_TRUE(record1_obj->IsEqual(record1_obj.get()));
  ASSERT_FALSE(record1_obj->IsEqual(record2_obj.get()));
}

TEST(RecordRdataTest, ParseARecord) {
  scoped_ptr<ARecordRdata> record_obj;

  // These are just the rdata portions of the DNS records, rather than complete
  // records, but it works well enough for this test.

  const char record[] = {
    '\x7F', '\x00', '\x00', '\x01'  // 127.0.0.1
  };

  DnsRecordParser parser(record, sizeof(record), 0);
  base::StringPiece record_strpiece(record, sizeof(record));

  record_obj = ARecordRdata::Create(record_strpiece, parser);
  ASSERT_TRUE(record_obj != NULL);

  ASSERT_EQ("127.0.0.1", IPAddressToString(record_obj->address()));

  ASSERT_TRUE(record_obj->IsEqual(record_obj.get()));
}

TEST(RecordRdataTest, ParseAAAARecord) {
  scoped_ptr<AAAARecordRdata> record_obj;

  // These are just the rdata portions of the DNS records, rather than complete
  // records, but it works well enough for this test.

  const char record[] = {
    '\x12', '\x34', '\x56', '\x78',
    '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x00',
    '\x00', '\x00', '\x00', '\x09'  // 1234:5678::9A
  };

  DnsRecordParser parser(record, sizeof(record), 0);
  base::StringPiece record_strpiece(record, sizeof(record));

  record_obj = AAAARecordRdata::Create(record_strpiece, parser);
  ASSERT_TRUE(record_obj != NULL);

  ASSERT_EQ("1234:5678::9",
            IPAddressToString(record_obj->address()));

  ASSERT_TRUE(record_obj->IsEqual(record_obj.get()));
}

TEST(RecordRdataTest, ParseCnameRecord) {
  scoped_ptr<CnameRecordRdata> record_obj;

  // These are just the rdata portions of the DNS records, rather than complete
  // records, but it works well enough for this test.

  const char record[] = {
    '\x03', 'w', 'w', 'w',
    '\x06', 'g', 'o', 'o', 'g', 'l', 'e',
    '\x03', 'c', 'o', 'm',
    '\x00'
  };

  DnsRecordParser parser(record, sizeof(record), 0);
  base::StringPiece record_strpiece(record, sizeof(record));

  record_obj = CnameRecordRdata::Create(record_strpiece, parser);
  ASSERT_TRUE(record_obj != NULL);

  ASSERT_EQ("www.google.com", record_obj->cname());

  ASSERT_TRUE(record_obj->IsEqual(record_obj.get()));
}

TEST(RecordRdataTest, ParsePtrRecord) {
  scoped_ptr<PtrRecordRdata> record_obj;

  // These are just the rdata portions of the DNS records, rather than complete
  // records, but it works well enough for this test.

  const char record[] = {
    '\x03', 'w', 'w', 'w',
    '\x06', 'g', 'o', 'o', 'g', 'l', 'e',
    '\x03', 'c', 'o', 'm',
    '\x00'
  };

  DnsRecordParser parser(record, sizeof(record), 0);
  base::StringPiece record_strpiece(record, sizeof(record));

  record_obj = PtrRecordRdata::Create(record_strpiece, parser);
  ASSERT_TRUE(record_obj != NULL);

  ASSERT_EQ("www.google.com", record_obj->ptrdomain());

  ASSERT_TRUE(record_obj->IsEqual(record_obj.get()));
}

TEST(RecordRdataTest, ParseTxtRecord) {
  scoped_ptr<TxtRecordRdata> record_obj;

  // These are just the rdata portions of the DNS records, rather than complete
  // records, but it works well enough for this test.

  const char record[] = {
    '\x03', 'w', 'w', 'w',
    '\x06', 'g', 'o', 'o', 'g', 'l', 'e',
    '\x03', 'c', 'o', 'm'
  };

  DnsRecordParser parser(record, sizeof(record), 0);
  base::StringPiece record_strpiece(record, sizeof(record));

  record_obj = TxtRecordRdata::Create(record_strpiece, parser);
  ASSERT_TRUE(record_obj != NULL);

  std::vector<std::string> expected;
  expected.push_back("www");
  expected.push_back("google");
  expected.push_back("com");

  ASSERT_EQ(expected, record_obj->texts());

  ASSERT_TRUE(record_obj->IsEqual(record_obj.get()));
}

TEST(RecordRdataTest, ParseNsecRecord) {
  scoped_ptr<NsecRecordRdata> record_obj;

  // These are just the rdata portions of the DNS records, rather than complete
  // records, but it works well enough for this test.

  const char record[] = {
    '\x03', 'w', 'w', 'w',
    '\x06', 'g', 'o', 'o', 'g', 'l', 'e',
    '\x03', 'c', 'o', 'm',
    '\x00',
    '\x00', '\x02', '\x40', '\x01'
  };

  DnsRecordParser parser(record, sizeof(record), 0);
  base::StringPiece record_strpiece(record, sizeof(record));

  record_obj = NsecRecordRdata::Create(record_strpiece, parser);
  ASSERT_TRUE(record_obj != NULL);

  ASSERT_EQ(16u, record_obj->bitmap_length());

  EXPECT_FALSE(record_obj->GetBit(0));
  EXPECT_TRUE(record_obj->GetBit(1));
  for (int i = 2; i < 15; i++) {
    EXPECT_FALSE(record_obj->GetBit(i));
  }
  EXPECT_TRUE(record_obj->GetBit(15));

  ASSERT_TRUE(record_obj->IsEqual(record_obj.get()));
}


}  // namespace net
