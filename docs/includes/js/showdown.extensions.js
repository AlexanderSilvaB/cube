showdown.setFlavor('github');
showdown.extension('targetlink', function() {
    return [ {
        type : 'lang',
        regex :
            /\[((?:\[[^\]]*]|[^\[\]])*)]\([ \t]*<?(.*?(?:\(.*?\).*?)?)>?[ \t]*((['"])(.*?)\4[ \t]*)?\)\{\:target=(["'])(.*?)\6}/g,
        replace : function(wholematch, linkText, url, a, b, title, c, target) {
            var result = '<a href="' + url + '"';

            if (typeof title != 'undefined' && title !== '' && title !== null)
            {
                title = title.replace(/"/g, '&quot;');
                title = showdown.helper.escapeCharacters(title, '*_', false);
                result += ' title="' + title + '"';
            }

            if (typeof target != 'undefined' && target !== '' && target !== null)
            {
                result += ' target="' + target + '"';
            }

            result += '>' + linkText + '</a>';
            return result;
        }
    } ];
});

showdown.extension('frame', function() {
    'use strict';
    var ext = {
        type : 'lang',
        regex : /\b(\\)?F\[?(.*?)\]?\((.*?)\)\B/g,
        replace : function(match, none, klass, link) {
            if (klass != undefined && klass != "")
                return '<iframe class="' + klass + '" src="' + link + '"></iframe>';
            else
                return '<iframe src="' + link + '"></iframe>';
        }
    };

    return [ ext ];
});
