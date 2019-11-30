function loadPage(page)
{
    $.get("pages/"+page+".md", function(data)
    {
        var converter = new showdown.Converter({extensions: ['targetlink']});
        var html = converter.makeHtml(data);
        $( ".markdown-body" ).html( html );
    }).fail(function() 
    {
        $( ".markdown-body" ).html( "<div class='error'>Failed to load this page.</div>" );
    }).always(function()
    {
        $( ".selected" ).removeClass("selected");
        $( "a[href='#"+page+"']" ).addClass("selected");
    });
}

$(function()
{
    loadPage("getting-started");

    $("#content > #menu > ul > li > a").click(function(e)
    {
        var page = this.href.split('#')[1];
        loadPage(page);
    });
});