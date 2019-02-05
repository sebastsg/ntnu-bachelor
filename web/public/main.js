function isExternalLink(link) {
    return link.search('http') !== -1;
}

function loadPage(link) {
    $('main > section').load('/page' + link, function(response, status, xhr) {
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
    if (link === undefined || isExternalLink(link) || $(this).hasClass('external')) {
        return;
    }
    event.preventDefault();
    loadPage(link);
});

function validateInput($element, name) {
    $element.data('saving', true);
    $element.data('dirty', false);
    $.ajax({
        method: 'get',
        url: '/api/' + name + '/' + $element.val(),
        dataType: 'json',
        success: response => {
            if ($element.data('dirty') === true) {
                validateInput($element, name);
                return;
            }
            if (response.is_taken === true) {
                $element.addClass('error');
            } else {
                $element.removeClass('error');
            }
            $element.data('saving', false);
            $element.data('dirty', false);
        }
    });
}

$(document).on('input', '#signup_form #display_name', function() {
    if ($(this).data('saving') === true) {
        $(this).data('dirty', true);
    } else {
        validateInput($(this), 'name_taken');
    }
});

$(document).on('input', '#signup_form #email', function() {
    if ($(this).data('saving') === true) {
        $(this).data('dirty', true);
    } else {
        validateInput($(this), 'email_registered');
    }
});

$(document).on('input', '#signup_form #password', function() {
    if ($(this).val().length < 8) {
        $(this).addClass('error');
    } else {
        $(this).removeClass('error');
    }
});

$(document).on('click', '#signup_form button', function() {
    $(this).prop('disabled', true);
    $.post('/post/signup', {
        display_name: $('#display_name').val(),
        email: $('#email').val(),
        password: $('#password').val()
    }, function(response) {
        $('main > section').append(response);
    });
});

$(window).bind('popstate', () => location.reload());
