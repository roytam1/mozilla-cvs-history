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

mysql > /dev/null 2>/dev/null << OK_ALL_DONE

use bugs;

drop table security
OK_ALL_DONE

mysql << OK_ALL_DONE
use bugs;
create table security (
        action varchar(255) not null primary key,
	flag mediumint not null
);

insert into security values ('priority',        1);
insert into security values ('bug_severity',    2);
insert into security values ('comments',        4);
insert into security values ('cc',              8);
insert into security values ('assigned_to',    16);
insert into security values ('bug_status',     32);
insert into security values ('version',        64);
insert into security values ('component',     128);
insert into security values ('rep_platform',  256);
insert into security values ('release',       512);
insert into security values ('bug_file_loc', 1024);
insert into security values ('short_desc',   2048);
insert into security values ('long_desc',    4096);
insert into security values ('resolution',   8192);
insert into security values ('internal',    16384);
insert into security values ('accept',      32768);

show columns from security;
show index from security;

OK_ALL_DONE
