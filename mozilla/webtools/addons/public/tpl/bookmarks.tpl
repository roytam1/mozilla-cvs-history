<div id="mBody">
<h1>Bookmarks Add-ons</h1>

<p>We've collected some of our favorite bookmark-related add-ons here for you to explore.  They can help you sync your bookmarks everywhere you use Firefox, keep things organized, or share your bookmarks with friends &#8211; or the whole world!</p>

<div class="addon-feature clearfix">
	<h1>{$primary.name} <span>by</span> <span class="author">{$primary.username}</span></h1>
	<img src="{$config.webpath}{$primary.previewuri}" alt="" class="addon-feature-image" />
	<div class="addon-feature-text corner-box">
		<p>{$primary.body}</p>
		<p class="install-button"><a href="{$primary.uri}" onclick="return install(event,'{$recommended[re].name} {$primary.version}', '{$config.webpath}/images/default.png', '{$primary.hash}');" title="Install {$primary.name} {$primary.version} (Right-Click to Download)"><span>Install now ({$primary.size} KB)</span></a></p>
	</div>
</div>

<h2>More bookmarks add-ons:</h2>
<p></p>
{section name=re loop=$other step=1 start=0}
	<div class="addon-feature clearfix">
		<img src="{$config.webpath}{$other[re].previewuri}" alt="" class="addon-feature-image" />
		<div class="addon-feature-text corner-box">
			<h4>{$other[re].name} <span>by {$other[re].username}</span></h4>
			<p>{$other[re].body}</p>
			<p class="install-button"><a href="{$other[re].uri}" onclick="return install(event,'{$recommended[re].name} {$other[re].version}', '{$config.webpath}/images/default.png', '{$other[re].hash}');" title="Install {$other[re].name} {$other[re].version} (Right-Click to Download)"><span>Install now ({$other[re].size} KB)</span></a></p>
		</div>
	</div>
{/section}

{include file="inc/search-box.tpl"}

