#!/usr/bin/perl -wT
# -*- Mode: perl; indent-tabs-mode: nil -*-
#
# The contents of this file are subject to the Mozilla Public
# License Version 1.1 (the "License"); you may not use this file
# except in compliance with the License. You may obtain a copy of
# the License at http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS
# IS" basis, WITHOUT WARRANTY OF ANY KIND, either express or
# implied. See the License for the specific language governing
# rights and limitations under the License.
#
# The Original Code is the Bugzilla Test Runner System.
#
# The Initial Developer of the Original Code is Maciej Maczynski.
# Portions created by Maciej Maczynski are Copyright (C) 2001
# Maciej Maczynski. All Rights Reserved.
#
# Contributor(s): Maciej Maczynski <macmac@xdsnet.pl>
#                 Ed Fuentetaja <efuentetaja@acm.org>
#                 Greg Hendricks <ghendricks@novell.com>

use strict;
use lib ".";

use Bugzilla;
use Bugzilla::Util;
use Bugzilla::Error;
use Bugzilla::Testopia::Util;
use Bugzilla::Testopia::TestPlan;
use Bugzilla::Testopia::TestRun;
use Bugzilla::Testopia::Environment::Category;
use Bugzilla::Testopia::Environment::Element;
use Bugzilla::Testopia::Environment::Property;

use vars qw($vars $template);
require "globals.pl";
my $dbh = Bugzilla->dbh;
my $cgi = Bugzilla->cgi;
my $template = Bugzilla->template;

push @{$::vars->{'style_urls'}}, 'testopia/css/default.css';

Bugzilla->login();
#Bugzilla->batch(1);
print $cgi->header;

my $action = $cgi->param('action') || '';
    
if ($action eq 'getversions'){
    my @prod_ids = split(",", $cgi->param('prod_ids'));
    my $tab = $cgi->param('current_tab') || '';
    my $plan = Bugzilla::Testopia::TestPlan->new({'plan_id'    => 0});
    my @validated;
    foreach my $p (@prod_ids){
        detaint_natural($p);
        validate_selection($p,'id','products');
        push @validated, $p if can_view_product($p);
    }
    my $prodlist = join(",", @validated);
    my $versions   = $plan->get_product_versions($prodlist) if ($tab eq 'run' || $tab eq 'plan');
    my $milestones = $plan->get_product_milestones($prodlist) if ($tab eq 'run');
    my $builds     = $plan->get_product_builds($prodlist) if ($tab eq 'run');
    my $components = $plan->get_product_components($prodlist) if ($tab eq 'case' || '');
    my $categories = $plan->get_product_categories($prodlist) if ($tab eq 'case' || '');

    my $ret = "<product>";
    foreach my $value (@$versions){
        $ret .= "<version>". xml_quote($value->{'name'}) ."</version>";
    }
    foreach my $value (@$milestones){
        $ret .= "<milestone>". xml_quote($value->{'name'}) ."</milestone>";
    }
    foreach my $value (@$builds){
        $ret .= "<build>". xml_quote($value->{'name'}) ."</build>";
    }
    foreach my $value (@$components){
        $ret .= "<component>". xml_quote($value->{'name'}) ."</component>";
    }
    foreach my $value (@$categories){
        $ret .= "<category>". xml_quote($value->{'name'}) ."</category>";
    }
    $ret .= "</product>";
    
    print $ret;
}

elsif ($action eq 'get_products'){
    my @prod;
    if (Param('useclassification')){
        my @classes = $cgi->param('class_ids');
        foreach my $id (@classes){
            my $class = Bugzilla::Classification->new($id);
            push @prod, @{$class->user_visible_products};
        }
    }
    else {
        @prod = Bugzilla->user->get_selectable_products;
    }
    
    my $ret;
    foreach my $e (@prod){
        $ret .= $e->{'id'}.'||'.$e->{'name'}.'|||';
    }
    chop($ret);
    print $ret;
}

elsif ($action eq 'get_categories'){
    my @prod_ids = $cgi->param('prod_id');
    my $ret;
    
    foreach my $prod_id (@prod_ids){
        detaint_natural($prod_id);
        my $cat = Bugzilla::Testopia::Environment::Category->new({});
        my $cats_ref = $cat->get_element_categories_by_product($prod_id);
        
        foreach my $e (@{$cats_ref}){
         	$ret .= $e->id.'||'.$e->name.'|||';
        }
    }
    chop($ret);
    print $ret;
}
elsif ($action eq 'get_elements'){
    my @cat_ids = $cgi->param('cat_id');
    my $ret;
    my @elmnts;
    
    foreach my $cat_id (@cat_ids){
        detaint_natural($cat_id);
        
        my $cat = Bugzilla::Testopia::Environment::Category->new($cat_id);
        my $elmnts_ref = $cat->get_elements_by_category($cat->{'name'});
        
        foreach my $e (@{$elmnts_ref}){
            push @elmnts, Bugzilla::Testopia::Environment::Element->new($e->{'element_id'});
        }
    }
    
    foreach my $e (@elmnts){
        $ret .= $e->id.'||'.$e->name.'|||';
    }
    chop($ret);
    print $ret;
}

elsif ($action eq 'get_properties'){
    my @elmnt_ids = $cgi->param('elmnt_id');
    my $ret;
    
    foreach my $elmnt_id (@elmnt_ids){
        detaint_natural($elmnt_id);
        
        my $elmnt = Bugzilla::Testopia::Environment::Element->new($elmnt_id);
        my $props = $elmnt->get_properties();
        
        foreach my $e (@{$props}){
            $ret .= $e->id.'||'.$e->name.'|||';
        }
    }
    chop($ret);
    print $ret;
}
elsif ($action eq 'get_valid_exp'){
    my @prop_ids = $cgi->param('prop_id');
    my $ret;
    
    foreach my $prop_id (@prop_ids){
        detaint_natural($prop_id);
        
        my $prop = Bugzilla::Testopia::Environment::Property->new($prop_id);
        my $exp = $prop->validexp;
    
    	my @exps = split /\|/, $exp;
	    foreach my $exp (@exps){
	        $ret .=$exp.'|||';
	    }
    }
    chop($ret);	
	print $ret;
}
else{
    my $plan = Bugzilla::Testopia::TestPlan->new({ 'plan_id' => 0 });
    my @allversions;
    foreach my $p (@{$plan->get_available_products}){
        push @allversions, @{$plan->get_product_versions($p->{'id'})};
    }
    # weed out any repeats
    my %versions;
    foreach my $v (@allversions){
        $versions{$v->{'id'}} = 1;
    }
    @allversions = ();
    foreach my $k (keys %versions){
        push @allversions, { 'id' => $k,  'name' => $k };
    }
    $vars->{'plan'} = $plan;
    $vars->{'product_versions'} = \@allversions;
    
    #TODO: Support default query
    my $tab = $cgi->param('current_tab') || '';
    if ($tab eq 'plan'){
        $vars->{'title'} = "Search For Test Plans";
    }
    elsif ($tab eq 'run'){
        my $run = Bugzilla::Testopia::TestRun->new({ 'run_id' => 0 });
        $vars->{'run'} = $run;
        $vars->{'title'} = "Search For Test Runs";
    }
    elsif ($tab eq 'environment'){
        $vars->{'classifications'} = Bugzilla->user->get_selectable_classifications;
        $vars->{'products'} = Bugzilla->user->get_selectable_products;
        $vars->{'env'} = Bugzilla::Testopia::Environment->new({'environment_id' => 0 });
        $vars->{'title'} = "Search For Test Run Environments";
    }
    else { # show the case form
        $tab = 'case';
        my $case = Bugzilla::Testopia::TestCase->new({ 'case_id' => 0 });
        $vars->{'case'} = $case;
        $vars->{'title'} = "Search For Test Cases";
    }
    
    $vars->{'current_tab'} = $tab;
    $template->process("testopia/search/advanced.html.tmpl", $vars)
        || ThrowTemplateError($template->error()); 
}
