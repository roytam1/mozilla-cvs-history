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

drop table versions
OK_ALL_DONE

mysql << OK_ALL_DONE
use bugs;
create table versions (
	value tinytext not null,
	program tinytext not null
);



insert into versions (value, program) values ("5.2", "Red Hat Linux");
insert into versions (value, program) values ("5.1", "Red Hat Linux");
insert into versions (value, program) values ("5.0", "Red Hat Linux");
insert into versions (value, program) values ("4.2", "Red Hat Linux");
insert into versions (value, program) values ("5.1", "Red Hat Powertools");
insert into versions (value, program) values ("2.0", "Red Hat Secure Web Server");
insert into versions (value, program) values ("1.0", "Red Hat Contrib|Net");
insert into versions (value, program) values ("2.1r", "bugzilla");

select * from versions;

show columns from versions;
show index from versions;

OK_ALL_DONE
