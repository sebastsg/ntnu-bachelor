function isExternalLink(link) {
    return link.search('http') !== -1;
}

function loadPage(link) {
    $('main > section').load('/ajax' + link, function(response, status, xhr) {
        if (xhr.status === 500) {
            console.error('An internal server error occurred.');
        } else if (xhr.status === 401) {
            document.location.href = '/';
        }
    });
    history.pushState({}, '', link);
}

$(document).on('click', 'a', function(event) {
    const link = $(this).attr('href');
    if (link === undefined || isExternalLink(link)) {
        return;
    }
    event.preventDefault();
    loadPage(link);
});

$(document).on('click', '.action[data-action=login]', function() {
    $.post('/post/login', {
        username: $('#username').val(),
        password: $('#password').val()
    }, function(response) {
        location.href = '/';
    });
});

$(window).bind('popstate', () => location.reload());
