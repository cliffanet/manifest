package WebMain;

use Clib::strict8;
use Clib::Const;
use Clib::Log;
use Clib::Web::Controller;
use Clib::Web::Param;
use Clib::Template::Package;
use Clib::DT;

$SIG{__DIE__} = sub { error('DIE: %s', $_) for @_ };

my $logpid = log_prefix($$);
my $logip = log_prefix('init');
my $loguser = log_prefix('');

my $href_prefix = '';
sub pref_short {
    my $href = webctrl_pref(@_);
    if (!defined($href)) {
        $href = '';
        #dumper 'pref undefined: ', \@_;
    }
    return $href;
}
sub pref { return $href_prefix . '/' . pref_short(@_); }

sub disp_search {
    my $href = shift() || return;
    
    if ($href_prefix ne '') {
        if (substr($href, 0, length($href_prefix)) ne $href_prefix) {
            return;
        }
        $href = substr($href, length($href_prefix), length($href)-length($href_prefix));
    }
    return webctrl_search($href);
}

webctrl_local(
        'CMain',
        attr => [qw/Title ReturnDebug ReturnText/],
        eval => "
            use Clib::Const;
            use Clib::Log;
            
            *wparam = *WebMain::param;
        ",
    ) || die webctrl_error;

my $param;
sub param { $param ||= Clib::Web::Param::param(prepare => 1, @_) }


sub init {
    my %p = @_;
    
    $logpid->set($$);
    
    $href_prefix = $p{href_prefix} if $p{href_prefix};
}

sub request {
    my $path = shift;
    
    # Инициализация запроса
    my $count = Clib::TimeCount->run();
    $logpid->set($$.'/'.$count);
    $logip->set($ENV{REMOTE_ADDR}||'-noip-');
    debug('request %s', $path);
    
    # Проверка указанного запроса
    my ($disp, @disp) = webctrl_search($path);
    my $logdisp;
    if ($disp) {
        debug('dispatcher found: %s (%s)', $disp->{symbol}, $disp->{path});
        #dumper 'disp: ', $disp;
        $logdisp = log_prefix($disp->{path});
    }
    else {
        error('dispatcher not found (redirect to /404): %s', $path);
        #$logdisp = log_prefix('-unknown dispatcher- ('.$path.')');
        #dumper \%ENV;
        return '', '404 Not found', 'Content-type' => 'text/plain', Pragma => 'no-cach';
    }
    
    # Выполняем обработчик
    my @web = ();
    {
        local $SIG{ALRM} = sub { error('Too long request do: %s (%s)', $disp->{path}, $path); };
        alarm(20);
        @web = webctrl_do($disp, @disp);
        alarm(0);
    }
    
    # Делаем вывод согласно типу возвращаемого ответа
    my %ret =
            map {
                my ($name, @p) = @$_;
                $name =~ /^return(.+)$/i ?
                    (lc($1) => [@p]) : ();
            }
            @{$disp->{attr}||[]};
        
    if ($ret{debug}) {
        require Data::Dumper;
        return
            join('',
                Data::Dumper->Dump(
                    [ $disp, \@disp, \@web, \%ENV, Clib::TimeCount->info],
                    [qw/ disp ARGS RETURN ENV RunCount RunTime /]
                )
            ),
            undef,
            'Content-type' => 'text/plain';
    }
    
    elsif ($ret{text})  {
        return return_text(@web);
    }
    
    else {
        return return_default(@web);
    };
}

sub clear {
    $logip->set('-clear-');
    undef $param;
    Clib::Web::Param::cookiebuild();
}

my $module_dir = c('template_module_dir');

if ($module_dir) {
    $module_dir = Clib::Proc::ROOT().'/'.$module_dir if $module_dir !~ /^\//;
}

my $proc;

sub tmpl_init {
    $proc && return $proc;
    
    my $log = log_prefix('Template::Package init');
    
    my %callback = (
        script_time     => \&Clib::TimeCount::interval,
        
        pref            => \&pref,
        #href_this       => sub {  },
        
        tmpl =>  sub {
            my $name = shift;
            my $tmpl = $proc->tmpl($name);
            if (!$tmpl) {
                error("tmpl('%s')> %s", $name, $proc->error());
            }
            return $tmpl;
        },
    );
    
    
    $proc = Clib::Template::Package->new(
        FILE_DIR    => Clib::Proc::ROOT().'/template',
        $module_dir ? (MODULE_DIR => $module_dir) : (),
        c('template_force_rebuild') ?
            (FORCE_REBUILD => 1) : (),
        USE_UTF8    => 1,
        CALLBACK    => \%callback,
        debug       => sub { debug(@_) }
    );
    
    if (!$proc) {
        error("on create obj: %s", $!||'-unknown-');
        return;
    }
    if (my $err = $proc->error) {
        error($err);
        undef $proc;
        return;
    };
    
    foreach my $plugin (qw/Base HTTP Block Misc/) {
        if (!$proc->plugin_add($plugin)) {
            error("plugin_add: %s", $proc->error);
            undef $proc;
            return;
        };
    }
    
    foreach my $parser (qw/Html/) {
        if (!$proc->parser_add($parser)) {
            error("parser_add: %s", $proc->error);
            undef $proc;
            return;
        };
    }
    
    $proc;
}

sub tmpl {
    $proc || tmpl_init() || return;
    
    my $tmpl = $proc->tmpl(@_);
    if (!$tmpl) {
        error("template(%s) compile: %s", $_[0], $proc->error);
        return;
    }
    
    return $tmpl;
}

sub return_html {
    my $base = shift;
    my $name = shift;
    my $block = shift;
    
    if (!$base && !$name) {
        return '', undef, 'Content-type' => 'text/html; charset=utf-8';
    }
    
    my $tmpl = $base ?
        tmpl($name, $base) :
        tmpl($name);
    $tmpl || return;
    
    my @p = (
        href_base   => pref(''),
        tmnow       => time(),
        ip          => $ENV{REMOTE_ADDR}||'',
        ver         => {
            original=> c('version'),
            full    => c('version') =~ /^(\d*\.\d)(\d*)([a-zA-Z])?$/ ? sprintf("%0.1f.%d%s", $1, $2 || 0, $3?'-'.$3:'') : c('version'),
            date    => Clib::DT::date(c('versionDate')),
        }
    );
    
    my $meth = 'html';
    $meth .= '_'.$block if $block;

    return
        $tmpl->$meth({ @_, @p, RUNCOUNT => Clib::TimeCount->count() }),
        undef,
        'Content-type' => 'text/html; charset=utf-8';
}

sub return_default { return return_html('', shift(), '', @_); }

sub return_text {
    my $text = join("\n", @_);
    
    return
        $text, undef, 'Content-type' => 'text/plain; charset=utf-8';
}

1;
