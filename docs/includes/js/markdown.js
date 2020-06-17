function loadPage(page)
{
    var url = "pages/" + page;
    if (!url.endsWith(".html"))
    {
        url = url + ".md";
    }
    $.get(url,
          function(data) {
              var converter = new showdown.Converter({extensions : [ 'targetlink', 'frame' ]});
              var html = converter.makeHtml(data);
              $(".markdown-body").html(html);
          })
        .fail(function() {
            $(".markdown-body").html("<div class='error'>Failed to load this page.</div>");
        })
        .always(function() {
            $(".selected").removeClass("selected");
            $("a[href='#" + page + "']").addClass("selected");
        });
}

$(function() {
    var page = window.location.hash.substr(1) || "getting-started";
    loadPage(page);

    $("#content > #menu > ul > li > a").click(function(e) {
        var page = this.href.split('#')[1];
        loadPage(page);
    });
});