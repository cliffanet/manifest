package CMain::Load;

use Clib::strict8;

use xls2fly;

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
    
    # Получаем предыдущий список
    my $memd = Cache::Memcached::Fast->new( c('MemCached') )
        || return rerr('Memcached init');
    
    my $fprev = $memd->get('flyday');
    if (
        # лююые изменения в документе будем проверять не только в случае
        # наличия предыдущей версии,
        (ref($fprev) eq 'HASH') && (ref($fprev->{flylist}) eq 'ARRAY') &&
        # но и при изменении $srcname,
        # т.к. если мы парсим совсем другой файл,
        # то получим кучу кривых "изменений", которых на самом деле нет
        $srcname && ($srcname eq $fprev->{srcname})
        ) {
        # Дополнительные параметры, основанные на разнице от прошлой загрузки
        xls2fly->by_prev($flylist, $fprev->{flylist});
        
        # Вычисляем изменеия и сообщаем о необходимости оповестить
        if (!$p->bool('nosave')) {
            my @log = xls2fly->pers2log($flylist, $fprev->{flylist});
            dumper log => \@log;
        }
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
            3600
        ) || return rerr('Memcached write');
    }
    
    my @opt = ();
    # доп. опции, которые запрашивает клиентская программа,
    # это какие-то сводные данные на основании полученного excel-файла
    # Мы их будем выводить в json-формате построчно после OK.
    foreach my $ol ($p->allstr('opt')) {
        foreach my $o (split(/\s*,\s*/, $ol)) {
            my $v = xls2fly->view($o)
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

sub form :
        Simple
{
    return 'form';
}

1;
