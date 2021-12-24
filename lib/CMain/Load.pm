package CMain::Load;

use Clib::strict8;

use xls2fly;

use Cache::Memcached::Fast;

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
    
    my $size = -s $fname;
    debug('file len: %d', $size);
    
    $size || return rerr('No file to load');
    
    # Парсим
    my $flylist = xls2fly->parse($fname, 'flylist');
    unlink $fname;
    
    $flylist || return rerr(xls2fly->err());
    
    # Дополнительные данные
    xls2fly->adding_data(@$flylist);
    
    # Получаем предыдущий список
    my $memd = Cache::Memcached::Fast->new( c('MemCached') )
        || return 'ERROR Memcached init';
    
    my $fprev = $memd->get('flyday');
    if ((ref($fprev) eq 'HASH') && (ref($fprev->{flylist}) eq 'ARRAY')) {
        # Дополнительные параметры, основанные на разнице от прошлой загрузки
        xls2fly->by_prev($flylist, $fprev->{flylist});
        
        # Вычисляем изменеия и сообщаем о необходимости оповестить
    }
    
    # Сохраняем
    $memd->set(
        flyday => {
                time    => time(),
                flylist => $flylist,
        },
        3600
    ) || return rerr('Memcached write');
    
    return 'OK';
}

sub form :
        Simple
{
    return 'form';
}

1;
