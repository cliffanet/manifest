package CMain::Load;

use Clib::strict8;

use xls2fly;

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
    my $dir = c('tmpdir') || return rerr('No `tmpdir`');
    if (!(-d $dir) && !mkdir($dir)) {
        return rerr('Can\'t make tmpdir');
    }
    
    my $fname = $dir.'/src.xlsx';
    my $p = wparam(file => { file => $fname });
    
    my $size = -s $fname;
    debug('file len: %d', $size);
    
    $size || return rerr('No file to load');
    
    my $flylist = xls2fly->parse($fname, 'flylist');
    unlink $fname;
    
    $flylist || return rerr(xls2fly->err());
    
    return 'OK';
}

sub form :
        Simple
{
    return 'form';
}

1;
