
<ul id="nav">
<li><span>Browse by Category</span></li>
{foreach key=id item=name from=$cats}
<li><a href="{$config.webpath}/search.php?cat={$id}&app={$app}&appfilter={$app}">{$name}</a></li>
{/foreach}
</ul>

