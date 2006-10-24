<h1>Frequently Asked Questions</h1>

<h2>What is Mozilla Add-ons?</h2>
<p>Mozilla Add-ons is the place to get updates and extras for
your <a href="http://www.mozilla.com/products/">Mozilla</a> products.</p>

<h2>How do I get involved?</h2>
<p class="first">We are looking for volunteers to help us with Add-ons. We are in need of 
people to review add-ons that get submitted to UMO. We especially need Mac and Thunderbird
users. If you are interested in being a part of this exciting project, please
join us in <kbd>#addons</kbd> on <kbd>irc.mozilla.org</kbd> to start getting a feeling for what's up or for a more informal chat.
</p>

<h2>What can I find here?</h2>
<dl>
<dt>Extensions</dt>
<dd>Extensions are small add-ons that add new functionality to your Firefox
web browser or Thunderbird email client.  They can add anything from
toolbars to completely new features.  Browse extensions for:
<a href="{$config.webpath}/firefox/extensions/">Firefox</a>, 
<a href="{$config.webpath}/thunderbird/extensions/">Thunderbird</a>,
<a href="{$config.webpath}/mozilla/extensions/">Mozilla Suite</a>
</dd>

<dt>Themes</dt>
<dd>Themes allow you to change the way your Mozilla program looks, with 
new graphics and colors. Browse themes for: 
<a href="{$config.webpath}/firefox/themes/">Firefox</a>, 
<a href="{$config.webpath}/thunderbird/themes/">Thunderbird</a>,
<a href="{$config.webpath}/mozilla/themes/">Mozilla Suite</a>
</dd>

<dt>Plugins</dt>
<dd>Plugins are programs that also add funtionality to your browser to
deliver specific content like videos, games, and music.  Examples of plugins
are Macromedia Flash Player, Adobe Acrobat, and Sun Microsystem's Java
Software.  Browse plug-ins for:
<a href="{$config.webpath}/{$app}/plugins/">Firefox &amp; Mozilla Suite</a>
</dd>
	</dl>

<dl>
{section name=faq loop=$faq}
<dt>{$faq[faq].title}</dt>
<dd>{$faq[faq].text|nl2br}</dd>
{/section}
</dl>

<h2>Valid Application Versions for Add-on Developers</h2>

<table class="appversions">
<tr>
    <th>Application Name</th>
    <th>Version</th>
    <th>GUID</th>
</tr>

{foreach item=app from=$appVersions}
<tr>
<td>{$app.appName}</td>
<td>{$app.displayVersion}</td>
<td>{$app.guid}</td></tr>
{/foreach}
</table>


<h2>I see this error when trying to upload my extension or theme: "The Name for your extension or theme already exists in the Update database."</h2>
<p>This is typically caused by mismatching GUIDs or a duplicate record.  If there is a duplicate record, chances are you should submit an update instead of trying to create a new add-on.  If you cannot see the existing record, then it is owned by another author, and you should consider renaming your add-on.</p>
