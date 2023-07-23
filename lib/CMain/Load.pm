package CMain::Load;

use Clib::strict8;

use xls2fly;
use flyinf;

use Cache::Memcached::Fast;
use JSON::XS;

########################################
###
###     Загрузка excel-файла
###

sub rerr {
    my $s = shift();
    $s = sprintf($s, @_) if @_;
    error($s);
    return 'ERROR '.$s;
}

sub _root :
        ReturnText
{
    # подготовка временной директории
    my $dir = c('tmpdir') || return rerr('No `tmpdir`');
    if (!(-d $dir) && !mkdir($dir)) {
        return rerr('Can\'t make tmpdir');
    }
    
    # Получение файла
    # Excel::Reader::XLSX не умеет работать с хендлами, только с физическим файлом
    my $fname = $dir.'/src.xlsx';
    my $p = wparam(file => { file => $fname });
    
    my $srcname = $p->str('file');
    my $size = -s $fname;
    debug('file: len=%d, name=%s', $size, $srcname);
    
    $size || return rerr('No file to load');
    
    # Парсим
    my $flylist = xls2fly->parse($fname, 'flylist');
    unlink $fname;
    
    $flylist || return rerr(xls2fly->err());
    
    # Дополнительные данные
    xls2fly->adding_data(@$flylist);
    
    # Временная БД
    my $memd = Cache::Memcached::Fast->new( c('MemCached') )
        || return rerr('Memcached init');
    
    # Получаем предыдущий список
    my $fprev = $memd->get('flyday');
    # Проверяем валидность предыдущих данных
    if (
        # любые изменения в документе будем проверять не только в случае
        # наличия предыдущей версии,
        (ref($fprev) ne 'HASH') || (ref($fprev->{flylist}) ne 'ARRAY') ||
        # но и при изменении $srcname,
        # т.к. если мы парсим совсем другой файл,
        # то получим кучу кривых "изменений", которых на самом деле нет
        !$srcname || ($srcname ne $fprev->{srcname})
        ) {
        # Если проверку не прошли
        undef $fprev;
    }
    
    # Дополнительные параметры, основанные на разнице от прошлой загрузки
    # Это операцию необходимо выполнить до сохранения в MemCached,
    # т.к. это всё ещё формирование исходных данных (с учётом предыдущих сохранений)
    if ($fprev) {
        xls2fly->by_prev($flylist, $fprev->{flylist});
    }
    
    # Сохраняем
    if ($p->bool('nosave')) {
        debug('option \'nosave\'');
    }
    else {
        $memd->set(
            flyday => {
                    time    => time(),
                    srcname => $srcname,
                    flylist => $flylist,
            },
            3600*4
        ) || return rerr('Memcached write');
    }

        
    # Оповещение об изменениях во взлётах
    # Вычисляем изменения и сообщаем о необходимости оповестить
    if ($fprev && !$p->bool('nosave')) {
        my @log = flyinf->pers2log($flylist, $fprev->{flylist});
        # Кто подписан, а кто нет, выяснит уже сам скрипт notify.xxx
        send_messages(@log) if @log;
    }
    
    my @opt = ();
    # доп. опции, которые запрашивает клиентская программа,
    # это какие-то сводные данные на основании полученного excel-файла
    # Мы их будем выводить в json-формате построчно после OK.
    foreach my $ol ($p->allstr('opt')) {
        foreach my $o (split(/\s*,\s*/, $ol)) {
            my $v = flyinf->view($o)
                || return rerr('Unknown required opt: %s', $o);
            
            my $r = $v->(@$flylist)
                || return rerr('Run-fail on required opt: %s', $o);
            my $json = eval { JSON::XS->new->pretty(0)->encode($r) }
                || return rerr('returned format wrong on required opt: %s', $o);
            # каждая опция будет возвращена отдельной строчкой
            # в ответе в формате:
            # OPTNAME { ... }
            push @opt, uc($o) . ' ' . $json;
        }
    }
    
    return OK => @opt;
}

sub send_messages {
    @_ || return;
    my $log = 
        eval { JSON::XS->new->utf8->pretty(0)->canonical->encode([ @_ ]) };
    $log || return;
    
    foreach my $type (qw/telegram/) {
        my $cmd = Clib::Proc::ROOT().'/bin/notify.'.$type;
        my $fh;
        if (!open($fh, "|$cmd")) {
            error('Can\'t exec \'%s\': %s', $cmd, $!);
            next;
        }
        print $fh $log;
        close $fh;
        
        debug('sended by %s - %d messages', $type, scalar(@_));
    }
    
    1;
}

sub form :
        Simple
{
    return 'form';
}

1;
