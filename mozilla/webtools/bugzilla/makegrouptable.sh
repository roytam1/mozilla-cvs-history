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

drop table groups
OK_ALL_DONE

mysql << OK_ALL_DONE
use bugs;
create table groups (
        groupid mediumint not null primary key,
        groupname varchar(255) not null,
	flags mediumint not null,
	goodfor bigint not null
);

insert into groups values (1, 'default',        0, 1800);
insert into groups values (2, 'support',      263, 86400);
insert into groups values (3, 'devel',      65535, 31536000);
insert into groups values (4, 'qa',        262143, 31536000);
insert into groups values (5, 'unlimited', 262143, 1800);
insert into groups values (6, 'comsup',         0, 86400);
insert into groups values (7, 'labs',       65535, 31536000);

show columns from groups;
show index from groups;

OK_ALL_DONE
