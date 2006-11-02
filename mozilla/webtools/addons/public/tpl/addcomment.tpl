{if $app eq "firefox"}
<h2><strong>{$addon->Name}</strong> &raquo; Add a Comment</h2>

{if $c_added_comment}
<p>Your comment has been added successfully.</p>
<ul>
<li><a href="{$config.webpath}/{$app}/{$addon->ID}/">Return to {$addon->Name|escape}</a></li>
</ul>
{else}
<p class="first"><a href="{$config.webpath}/{$app}/{$addon->ID}/">&laquo; Back to {$addon->Name}</a></p>
<form id="commentform" class="amo-form" name="commentform" method="post" action="">

    <p>All fields are required.</p>

    <div>
    <label class="amo-label-small" for="c_rating">Rating:</label>
    <select id="c_rating" name="c_rating">
        {html_options values=$rate_select_value output=$rate_select_name selected=$c_rating_value}
    </select>
    {if $c_errors.c_rating}
        <div class="amo-form-error">
            Please choose a rating.
        </div>
    {/if}
    </div>

    <div>
    <label class="amo-label-small" for="c_title">Title:</label>
    <input type="text" id="c_title" name="c_title" value="{$c_title_value|escape}" size="36" maxlength="255"/>
    {if $c_errors.c_title}
        <div class="amo-form-error">
            Please provide a title with your comments.
        </div>
    {/if}
    </div>

    <div>
    <label class="amo-label-small" for="c_comments">Comments:</label>
    <textarea id="c_comments" name="c_comments" cols="31" rows="7">{$c_comments_value|escape}</textarea>
    {if $c_errors.c_comments}
        <div class="amo-form-error">
            Please include some valid comments.
        </div>
    {/if}
    </div>

    <div>
    <input class="amo-submit" type="submit" id="c_submit" name="c_submit" value="Submit Comment &raquo;" />
    </div>

</form>
{/if}
{else} {* end firefox, begin non-firefox *}
<div class="rating" title="{$addon->Rating} out of 5">Rating: {$addon->Rating}</div>
<h2><strong>{$addon->Name}</strong> &raquo; Add a Comment</h2>

<p class="first">
<strong><a href="{$config.webpath}/{$app}/{$addon->ID}/">{$addon->Name} {$addon->Version}</a></strong>,
by
{foreach key=key item=item from=$addon->Authors}
    <a href="{$config.webpath}/{$app}/{$item.UserID|escape}/author/">{$item.UserName|escape}</a>,
{/foreach}
released on {$addon->VersionDateAdded|date_format}
</p>

{if $c_added_comment}
<p>Your comment has been added successfully.</p>
<ul>
<li><a href="{$config.webpath}/{$app}/{$addon->ID}/">Return to {$addon->Name|escape}</a></li>
</ul>
{else}
<form id="commentform" class="amo-form" name="commentform" method="post" action="">

    <p>All fields are required.</p>

    <div>
    <label class="amo-label-small" for="c_rating">Rating:</label>
    <select id="c_rating" name="c_rating">
        {html_options values=$rate_select_value output=$rate_select_name selected=$c_rating_value}
    </select>
    {if $c_errors.c_rating}
        <div class="amo-form-error">
            Please choose a rating.
        </div>
    {/if}
    </div>

    <div>
    <label class="amo-label-small" for="c_title">Title:</label>
    <input type="text" id="c_title" name="c_title" value="{$c_title_value|escape}" size="36" maxlength="255"/>
    {if $c_errors.c_title}
        <div class="amo-form-error">
            Please provide a title with your comments.
        </div>
    {/if}
    </div>

    <div>
    <label class="amo-label-small" for="c_comments">Comments:</label>
    <textarea id="c_comments" name="c_comments" cols="31" rows="7">{$c_comments_value|escape}</textarea>
    {if $c_errors.c_comments}
        <div class="amo-form-error">
            Please include some valid comments.
        </div>
    {/if}
    </div>

    <div>
    <input class="amo-submit" type="submit" id="c_submit" name="c_submit" value="Submit Comment &raquo;" />
    </div>

</form>
{/if}
{/if} {* end not-firefox *}
