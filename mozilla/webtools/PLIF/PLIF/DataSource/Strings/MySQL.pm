# -*- Mode: perl; tab-width: 4; indent-tabs-mode: nil; -*-
#
# This file is MPL/GPL dual-licensed under the following terms:
#
# The contents of this file are subject to the Mozilla Public License
# Version 1.1 (the "License"); you may not use this file except in
# compliance with the License. You may obtain a copy of the License at
# http://www.mozilla.org/MPL/
#
# Software distributed under the License is distributed on an "AS IS"
# basis, WITHOUT WARRANTY OF ANY KIND, either express or implied. See
# the License for the specific language governing rights and
# limitations under the License.
#
# The Original Code is PLIF 1.0.
# The Initial Developer of the Original Code is Ian Hickson.
#
# Alternatively, the contents of this file may be used under the terms
# of the GNU General Public License Version 2 or later (the "GPL"), in
# which case the provisions of the GPL are applicable instead of those
# above. If you wish to allow use of your version of this file only
# under the terms of the GPL and not to allow others to use your
# version of this file under the MPL, indicate your decision by
# deleting the provisions above and replace them with the notice and
# other provisions required by the GPL. If you do not delete the
# provisions above, a recipient may use your version of this file
# under either the MPL or the GPL.

package PLIF::DataSource::Strings::MySQL;
use strict;
use vars qw(@ISA);
use PLIF::DataSource::Strings;
@ISA = qw(PLIF::DataSource::Strings);
1;

sub databaseType {
    return qw(mysql);
}

__DATA__

sub getString {
    my $self = shift;
    my($app, $variant, $string) = @_;
    return $self->database($app)->execute('SELECT type, version, data FROM strings WHERE variant = ? AND name = ?', $variant, $string)->row;
}

sub getVariants {
    my $self = shift;
    my($app, $protocol) = @_;
    return $self->database($app)->execute('SELECT id, quality, type, encoding, charset, language FROM stringVariants WHERE protocol = ?', $protocol)->rows;
}

sub getDescribedVariants {
    my $self = shift;
    my($app) = @_;
    my %result = ();
    foreach my $variant ($self->database($app)->execute('SELECT id, name, protocol, quality, type, encoding, charset, language, description, translator FROM stringVariants')->rows) {
        $result{$variant->[0]} = {
            'name' => $variant->[1],
            'protocol' => $variant->[1],
            'quality' => $variant->[2],
            'type' => $variant->[3],
            'encoding' => $variant->[4],
            'charset' => $variant->[5],
            'language' => $variant->[6],
            'description' => $variant->[7],
            'translator' => $variant->[8],
        };
    }
    return %result;
}

sub getVariant {
    my $self = shift;
    my($app, $id) = @_;
    return $self->database($app)->execute('SELECT name, protocol, quality, type, encoding, charset, language, description, translator FROM stringVariants WHERE id = ?', $id)->row;
}

sub getVariantStrings {
    my $self = shift;
    my($app, $variant) = @_;
    my %result = ();
    foreach my $string ($self->database($app)->execute('SELECT name, type, version, data FROM strings WHERE variant = ?', $variant)->rows) {
        $result{$string->[0]} = [$string->[1], $string->[2]];
    }
    return %result;
}

sub getStringVariants {
    my $self = shift;
    my($app, $string) = @_;
    my %result = ();
    foreach my $variant ($self->database($app)->execute('SELECT variant, type, version, data FROM strings WHERE name = ?', $string)->rows) {
        $result{$variant->[0]} = [$variant->[1], $variant->[2]];
    }
    return %result;
}

sub getAllStringVersions {
    my $self = shift;
    my($app) = @_;
    return $self->database($app)->execute('SELECT stringVariants.id, stringVariants.name, stringVariants.protocol, strings.name, strings.version FROM stringVariants, strings WHERE stringVariants.id = strings.variant')->rows;
}

sub setVariant {
    my $self = shift;
    my($app, $id, $name, $protocol, $quality, $type, $encoding, $charset, $language, $description, $translator) = @_;
    if (defined($id)) {
        $self->database($app)->execute('UPDATE stringVariants SET name=?, protocol=?, quality=?, type=?, encoding=?, charset=?, language=?, description=?, translator=? WHERE id = ?', $name, $protocol, $quality, $type, $encoding, $charset, $language, $description, $translator, $id);
    } else {
        return $self->database($app)->execute('INSERT INTO stringVariants SET name=?, protocol=?, quality=?, type=?, encoding=?, charset=?, language=?, description=?, translator=?', $name, $protocol, $quality, $type, $encoding, $charset, $language, $description, $translator)->MySQLID;
    }
}

sub setString {
    my $self = shift;
    my($app, $variant, $version, $string, $type, $data) = @_;
    if (defined($data)) {
        $self->database($app)->execute('REPLACE INTO stringVariants SET variant=?, string=?, type=?, version=?, data=?', $variant, $string, $type, $version, $data);
    } else {
        $self->database($app)->execute('DELETE FROM stringVariants WHERE variant = ? AND string = ?', $variant, $string);
    }
}

sub setupInstall {
    my $self = shift;
    my($app) = @_;
    my $helper = $self->helper($app);
    $self->dump(9, 'about to configure string data source...');
    if (not $helper->tableExists($app, $self->database($app), 'stringVariants')) {
        $app->output->setupProgress('dataSource.strings.stringVariants');
        $self->database($app)->execute('
            CREATE TABLE stringVariants (
                                         id integer unsigned auto_increment NOT NULL PRIMARY KEY,
                                         name varchar(255) NOT NULL,
                                         protocol varchar(255) NOT NULL,
                                         encoding varchar(255),
                                         type varchar(255) NOT NULL,
                                         charset varchar(255),
                                         language varchar(255) NOT NULL,
                                         quality float NOT NULL default 1.0,
                                         description text,
                                         translator varchar(255),
                                         UNIQUE KEY (name)
                                         )
        ');
    } else {
        # check its schema is up to date
    }
    if (not $helper->tableExists($app, $self->database($app), 'strings')) {
        $app->output->setupProgress('dataSource.strings.strings');
        $self->database($app)->execute('
            CREATE TABLE strings (
                                  variant integer unsigned NOT NULL,
                                  name varchar(32) NOT NULL,
                                  type varchar(32) NOT NULL,
                                  version varchar(32) NOT NULL,
                                  data text,
                                  PRIMARY KEY (variant, name)
                                  )
        ');
    } else {
        # check its schema is up to date
        if (not $helper->columnExists($app, $self->database($app), 'strings', 'type')) {
            $self->database($app)->execute('ALTER TABLE strings ADD COLUMN type varchar(32) NOT NULL');
        }
        if (not $helper->columnExists($app, $self->database($app), 'strings', 'version')) {
            $self->database($app)->execute('ALTER TABLE strings ADD COLUMN version varchar(32) NOT NULL');
        }
    }
    $self->dump(9, 'done configuring string data source');
    return $self->SUPER::setupInstall(@_);
}
