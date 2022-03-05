package CMain::PPU;

use Clib::strict8;
use Encode;

sub _root :
        Simple
{
    my $file = c('ppufile')
        || return 'ppu', err => 'Const `ppufile` not defined';
    
    if (!(-e $file)) {
        return 'ppu';
    }
    
    my $fh;
    open($fh, $file)
        || return 'ppu', err => 'Can\'t open \''.$file.'\': '.$!;
    
    local $/ = undef;
    my $data = <$fh>;
    Encode::_utf8_on($data);
    close $fh;
    
    return
        'ppu',
        file => $data;
}

sub set :
        Simple
{
    my $file = c('ppufile')
        || return 'ppu', err => 'Const `ppufile` not defined';
    
    my $p = wparam();
    $p->exists('ppu') || return _root();
    my $data = $p->raw('ppu');
    
    my $fh;
    open($fh, '>', $file)
        || return 'ppu', file => $data, err => 'Can\'t open \''.$file.'\': '.$!;
    
    Encode::_utf8_off($data);
    print $fh $data;
    close $fh;
    
    return _root(), saveok => 1;
}

1;
