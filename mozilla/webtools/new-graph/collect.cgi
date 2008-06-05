#!/usr/bin/env python

import cgitb; cgitb.enable()

import sys
import cgi
import time
import re

from graphsdb import db

#if var is a valid number returns a value other than None
def checkNumber(var):
    if var is None:
      return 1
    reNumber = re.compile('^[0-9.]*$')
    return reNumber.match(var)

#if var is a valid string returns a value other than None
def checkString(var):
    if var is None:
      return 1
    reString = re.compile('^[0-9A-Za-z._()\- ]*$')
    return reString.match(var)

print "Content-type: text/plain\n\n"
link_format = "RETURN:%.2f:%sspst=range&spstart=%d&spend=%d&bpst=cursor&bpstart=%d&bpend=%d&m1tid=%d&m1bl=0&m1avg=0\n"
link_str = ""


form = cgi.FieldStorage()

# incoming query string has the following parameters:
# type=discrete|continuous
#  indicates discrete vs. continuous dataset, defaults to continuous
# value=n
#  (REQUIRED) value to be recorded as the actual test value
# tbox=foopy
#  (REQUIRED) name of the tinderbox reporting the value (or rather, the name that is to be given this set of data)
# testname=test
#  (REQUIRED) the name of this test
# data=rawdata
#  raw data for this test
# time=seconds
#  time since the epoch in GMT of this test result; if ommitted, current time at time of script run is used
# date
#  date that the test was run - this is for discrete graphs
# branch=1.8.1,1.8.0 or 1.9.0
#  name of the branch that the build was generated for
# branchid=id
#  date of the build 
#  http://wiki.mozilla.org/MozillaQualityAssurance:Build_Ids

#make sure that we are getting clean data from the user
for strField in ["type", "data", "tbox", "testname", "branch", "branchid"]:
    val = form.getfirst(strField)
    if not checkString(val):
        print "Invalid string arg: ", strField, " '" + val + "'"
        sys.exit(500)
    globals()[strField] = val

for numField in ["date", "time", "value"]:
    val = form.getfirst(numField)
    if numField == "time":
        numField = "timeval"
    if not checkNumber(val):
        print "Invalid num arg: ", numField, " '" + val + "'"
        sys.exit(500)
    globals()[numField] = val

#do some checks to ensure that we are enforcing the requirement rules of the script
if (not type):
    type = "continuous"

if (not timeval):
    timeval = int(time.time())

if (type == "discrete") and (not date):
   print "Bad args, need a valid date"
   sys.exit(500)

if (not value) or (not tbox) or (not testname):
    print "Bad args"
    sys.exit(500)


# Create the DB schema if it doesn't already exist
# XXX can pull out dataset_info.machine and dataset_info.{test,test_type} into two separate tables,
# if we need to.

# figure out our dataset id
setid = -1

while setid == -1:
    cur = db.cursor()
    cur.execute("SELECT id FROM dataset_info WHERE type <=> ? AND machine <=> ? AND test <=> ? AND test_type <=> ? AND extra_data=? AND branch <=> ? AND date <=> ?",
                (type, tbox, testname, "perf", "branch="+branch, branch, date))
    res = cur.fetchall()
    cur.close()

    if len(res) == 0:
        db.execute("INSERT INTO dataset_info (type, machine, test, test_type, extra_data, branch, date) VALUES (?,?,?,?,?,?,?)",
                   (type, tbox, testname, "perf", "branch="+branch, branch, date))
    else:
        setid = res[0][0]

#determine if we've seen this set of data before
if (type == "discrete" and int(timeval) == 0):
     cur = db.cursor()
     cur.execute("SELECT dataset_id FROM dataset_values WHERE dataset_id = ? AND time = ?", (setid, timeval))
     res = cur.fetchall()
     cur.close
     if len(res) <> 0:
         print "found a matching discrete data set"
         db.execute("DELETE FROM dataset_values WHERE dataset_id = ?", (setid,))
         db.execute("DELETE FROM dataset_branchinfo WHERE dataset_id = ?", (setid,))
         db.execute("DELETE FROM dataset_extra_data WHERE dataset_id = ?", (setid,))
         db.execute("DELETE FROM annotations WHERE dataset_id = ?", (setid,))
elif (type == "continuous"):
     cur = db.cursor()
     cur.execute("SELECT dataset_id FROM dataset_values WHERE dataset_id = ? AND time = ?", (setid, timeval))
     res = cur.fetchall()
     cur.close
     if len(res) <> 0:
         print "found a matching continuous data point"
         db.execute("DELETE FROM dataset_values WHERE dataset_id = ? AND time = ?", (setid, timeval))
         db.execute("DELETE FROM dataset_branchinfo WHERE dataset_id = ? AND time = ?", (setid, timeval))
         db.execute("DELETE FROM dataset_extra_data WHERE dataset_id = ? AND time = ?", (setid, timeval))
         db.execute("DELETE FROM annotations WHERE dataset_id = ? AND time = ?", (setid, timeval))

db.execute("INSERT INTO dataset_values (dataset_id, time, value) VALUES (?,?,?)", (setid, timeval, value))
db.execute("INSERT INTO dataset_branchinfo (dataset_id, time, branchid) VALUES (?,?,?)", (setid, timeval, branchid))
if data and data != "":
    db.execute("INSERT INTO dataset_extra_data (dataset_id, time, data) VALUES (?,?,?)", (setid, timeval, data))

cur = db.cursor()
cur.execute("SELECT MIN(time), MAX(time) FROM dataset_values WHERE dataset_id = ?", (setid,))
res = cur.fetchall()
cur.close()
tstart = res[0][0]
tend = res[0][1]
if type == "discrete":
    link_str += (link_format % (float(-1), "dgraph.html#name=" + testname + "&", tstart, tend, tstart, tend, setid,))
else:
    tstart = 0
    link_str += (link_format % (float(-1), "graph.html#",tstart, tend, tstart, tend, setid,))


#this code auto-adds a set of continuous data for each series of discrete data sets - creating an overview of the data
# generated by a given test (matched by machine, test, test_type, extra_data and branch) 
# it is not terribly efficient as it updates the tracking number on the continuous set each time a point is added
# to the discrete set.  If the efficiency becomes a concern we can re-examine the code - for now it is a good
# solution for generating the secondary, tracking data.
if  type == "discrete" :
    timeval = date
    date = ''
    cur = db.cursor()
    #throw out the largest value and take the average of the rest
    cur.execute("SELECT AVG(value) FROM dataset_values WHERE dataset_id = ? and value != (SELECT MAX(value) from dataset_values where dataset_id = ?)", (setid, setid,))
    res = cur.fetchall()
    cur.close()
    avg = res[0][0]
    if avg is not None:

        setid = -1 
        while setid == -1 :
            cur = db.cursor()
            cur.execute("SELECT id from dataset_info where type <=> ? AND machine <=> ? AND test <=> ? AND test_type <=> ? AND extra_data <=> ? AND branch <=> ? AND date <=> ?",
                    ("continuous", tbox, testname+"_avg", "perf", "branch="+branch, branch, date))
            res = cur.fetchall()
            cur.close()
            if len(res) == 0:
                db.execute("INSERT INTO dataset_info (type, machine, test, test_type, extra_data, branch, date) VALUES (?,?,?,?,?,?,?)",
                       ("continuous", tbox, testname+"_avg", "perf", "branch="+branch, branch, date))
            else:
                setid = res[0][0]
        cur = db.cursor()
        cur.execute("SELECT * FROM dataset_values WHERE dataset_id=? AND time <=> ?", (setid, timeval))
        res = cur.fetchall()
        cur.close()
        if len(res) == 0:
            db.execute("INSERT INTO dataset_values (dataset_id, time, value) VALUES (?,?,?)", (setid, timeval, avg))
            db.execute("INSERT INTO dataset_branchinfo (dataset_id, time, branchid) VALUES (?,?,?)", (setid, timeval, branchid))
        else:
            db.execute("UPDATE dataset_values SET value=? WHERE dataset_id=? AND time <=> ?", (avg, setid, timeval))
            db.execute("UPDATE dataset_branchinfo SET branchid=? WHERE dataset_id=? AND time <=> ?", (branchid, setid, timeval))
        cur = db.cursor()
        cur.execute("SELECT MIN(time), MAX(time) FROM dataset_values WHERE dataset_id = ?", (setid,))
        res = cur.fetchall()
        cur.close()
        tstart = 0
        tend = res[0][1]
        link_str += (link_format % (float(avg), "graph.html#", tstart, tend, tstart, tend, setid,))

db.commit()
print "Inserted."
print link_str

sys.exit()
