DOC_ROOT = /home/brad/sandbox/bugzilla/mozilla/webtools/bugzilla/root
BIN_DIR  = /home/brad/sandbox/bugzilla/mozilla/webtools/bugzilla/bin

IMAGE_FILES =			\
	1x1.gif			\
	ant.jpg

BIN_FILES =			\
	whineatnews.pl		\
	collectstats.pl

INCLUDE_FILES =			\
	CGI.pl			\
	bug_form.pl		\
	defparams.pl		\
	globals.pl

HTML_FILES =			\
	bug_status.html		\
	help.html		\
	helpemailquery.html	\
	how_to_mail.html	\
	index.html		\
	newquip.html		\
	notargetmilestone.html

CGI_FILES =			\
	backdoor.cgi		\
	buglist.cgi		\
	changepassword.cgi	\
	colchange.cgi		\
	describecomponents.cgi	\
	doeditowners.cgi	\
	doeditparams.cgi	\
	editowners.cgi		\
	editparams.cgi		\
	enter_bug.cgi		\
	long_list.cgi		\
	new_comment.cgi		\
	post_bug.cgi		\
	process_bug.cgi		\
	query.cgi		\
	relogin.cgi		\
	reports.cgi		\
	sanitycheck.cgi		\
	show_activity.cgi	\
	show_bug.cgi

all:  install_html install_cgi install_bin install_include install_images

install_images:
	@for file in $(IMAGE_FILES); do	\
	  cp $$file ${DOC_ROOT}/$$file;	\
	done

install_html:
	@for file in $(HTML_FILES); do	\
	  cp $$file ${DOC_ROOT}/$$file;	\
	done

install_include:
	@for file in $(INCLUDE_FILES); do	\
	  /usr/ucb/sed \
	    -e 's!@''DB_HOST''@!stratosphere.cybersight.com:3307!g' \
	    -e 's!@''DB_DATABASE''@!bugzilla!g' \
	    -e 's!@''DB_USERID''@!bugzilla!g' \
	    -e 's!@''DB_PASSWORD''@!adg1Qet!g' \
	    $$file > ${DOC_ROOT}/$$file; \
	done

install_cgi:
	@for file in $(CGI_FILES); do	\
	  /usr/ucb/sed -e 's!@''PERL5''@!/usr/local/bin/perl!g' $$file > ${DOC_ROOT}/$$file; \
	  /usr/local/bin/chmod +x ${DOC_ROOT}/$$file; \
	done

install_bin:
	@for file in $(BIN_FILES); do	\
	  /usr/ucb/sed -e 's!@''PERL5''@!/usr/local/bin/perl!g' $$file > ${BIN_DIR}/$$file; \
	  /usr/local/bin/chmod +x ${BIN_DIR}/$$file; \
	done
