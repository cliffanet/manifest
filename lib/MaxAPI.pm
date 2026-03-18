package MaxAPI;

use strict;
use warnings;
use utf8;

use Clib::Const;
use Clib::Log;

use Encode;
use JSON::XS;
use LWP::UserAgent;

our $isreq;
sub isreq { $isreq; }

sub req {
    my ($method, $cmd, $json) = @_;

    local $SIG{__DIE__} = undef;
    local $isreq = 1;

    # наш токен
    my $token = c('max_token');
    if (!$token) {
        error('`max_token` not defined');
        return;
    }

    my $url = 'https://platform-api.max.ru/' . $cmd;

    # http prepare
    debug('api: %s => %s', $method, $cmd);
    my $http = eval {
        my $r = HTTP::Request->new($method, $url);
        $r->header(Authorization => $token);
        if ($json) {
            if (ref $json) {
                $json = JSON::XS->new->pretty(0)->encode( $json );
            }
            Encode::_utf8_off($json);
            $r->content_type('application/json');
            $r->content($json);
            debug('api content: %s', $json);
        }
        $r;
    };
    if (!$http) {
        error('api prepare: %s', $@);
        return;
    }

    my $r = eval {
        my $ua = LWP::UserAgent->new;
        $ua->request($http);
    };
    $r || return;

    my $code = $r->code();
    if ($code != 200) {
        error('api error: [%s] %s', $code, $r->decoded_content());
        return;
    }
    my $raw  = $r->decoded_content();
    Encode::_utf8_off($raw);
    my $data = eval { JSON::XS->new->utf8->decode( $raw ) };
    if (!$data && $@) {
        error('api json err: %s', $@);
    }

    debug('api reply: %s', int $code);

    return $data;
}

sub get {
    shift() if ($_[0]||'') eq __PACKAGE__;
    my $cmd = shift;
    $cmd = sprintf($cmd, @_) if @_;

    return req(GET => $cmd);
}

sub post {
    shift() if ($_[0]||'') eq __PACKAGE__;
    my $cmd = shift;

    if (ref($_[0]) eq 'ARRAY') {
        $cmd = sprintf($cmd, @{ shift() });
    }

    return req(POST => $cmd, { @_} );
}

sub _data2url {
    my @s = ();
    
    while (@_) {
        my $k = _url_encode(shift());
        my $v = _url_encode(shift());
        push @s, $k . '=' . $v;
    }
    
    return join('&', @s);
}

sub _url_encode {
    my $string = shift;
    
    Encode::_utf8_off($string);
    $string =~ s/([^-.\w ])/sprintf('%%%02X', ord $1)/ge;
    $string =~ tr/ /+/;

    return $string;
}

sub me {
    return get('me');
}

sub updates {
    shift() if ($_[0]||'') eq __PACKAGE__;
    my $q = _data2url(@_);

    return get('updates%s', $q ? '?'.$q : '');
}

sub msgtxt {
    shift() if ($_[0]||'') eq __PACKAGE__;
    my $userid = shift();
    my $txt = shift() || return;
    $txt = sprintf($txt, @_) if @_;

    return post('messages?user_id=%d', [$userid], text => $txt);
}

1;
