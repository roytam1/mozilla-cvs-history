#!/bin/sh
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.0 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
# 
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See the
# License for the specific language governing rights and limitations
# under the License.
# 
# The Original Code is the Bugzilla Bug Tracking System.
# 
# The Initial Developer of the Original Code is Netscape Communications
# Corporation. Portions created by Netscape are Copyright (C) 1998
# Netscape Communications Corporation. All Rights Reserved.
# 
# Contributor(s): Terry Weissman <terry@mozilla.org>
#                 Andrew Anderson <andrew@redhat.com>

mysql > /dev/null 2>/dev/null << OK_ALL_DONE

use bugs;

drop table bugs;
OK_ALL_DONE

mysql << OK_ALL_DONE
use bugs;
create table bugs (
bug_id int8 not null auto_increment primary key,
group_id mediumint not null,
assigned_to mediumint not null,
bug_file_loc text,
patch_file_loc text,
bug_severity enum("security", "high", "normal", "low") not null,
bug_status enum("NEW", "VERIFIED", "ASSIGNED", "REOPENED", "RESOLVED") not null,
view tinyint,
creation_ts datetime,
delta_ts timestamp,
short_desc mediumtext,
long_desc mediumtext,
op_sys tinytext,
priority enum("high", "normal", "low") not null,
product varchar(255) not null,
rep_platform enum("All", "alpha", "m68k", "i386", "mips", "sparc", "sparc64", "ppc", "Other"),
reporter mediumint not null,
version varchar(16) not null,
release varchar(16) not null,
component varchar(50) not null,
resolution enum("", "FIXED", "DISCARD", "WONTFIX", "LATER", "REMIND", "DUPLICATE", "WORKSFORME") not null,
class enum("install/upgrade", "packaging", "functionality", "security", "documentation") not null,

index (assigned_to),
index (delta_ts),
index (bug_severity),
index (bug_status),
index (priority),
index (product),
index (reporter),
index (version),
index (component),
index (resolution)

);

show columns from bugs;
show index from bugs;


OK_ALL_DONE
