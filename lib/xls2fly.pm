package xls2fly;

use strict;
use warnings;
use utf8;

use Clib::Const;
use Clib::Log;

use Excel::Reader::XLSX;

####################################################

my $error;
sub err {
    shift() if $_[0] && (($_[0] eq __PACKAGE__) || (ref($_[0]) eq __PACKAGE__));
    
    if (@_) {
        $error = shift();
        $error = sprintf($error, @_) if @_;
        #error($error);
        return;
    }
    
    return $error;
}

sub parse {
    shift() if $_[0] && (($_[0] eq __PACKAGE__) || (ref($_[0]) eq __PACKAGE__));
    my $log = log_prefix('xls2fly->parse()');
    
    # Читаем исходный файл
    my $fsrc = shift()
        || return err('No file to load');
    
    my $reader   = Excel::Reader::XLSX->new();
    $reader->{_tempdir} = c('tmpdir')
        || return err('No `tmpdir`');
    
    my $workbook = eval { $reader->read_file( $fsrc ) };
    
    $workbook
        || return err('[Excel::Reader::XLSX->read_file()] %s', $reader->error() || $@ || 'read fail');
    my $tmpdir = $reader->{_package_dir};
    
    my $sheetno = 0;
    # Какие данные нам нужны
    my %use =
        wantarray ? 
            (map { ($_ => 1) } @_) :
        @_ ?
            ($_[0] => 1) :
            ();
    my %r = ();
    
    # Взлеты
    my $flylist = ($r{flylist} = []);
    while ($use{flylist}) {
        my $flySheet = c(flySheet => $sheetno) || last;
    
        my $worksheet = $workbook->worksheet($sheetno);
        
        my $prev_title = undef;
        my ($prev_spec, $prev_last);
        my $fly;
        my $fnum = 0;
        while ( my $row = $worksheet->next_row() ) {
            my @row = ();
            my $rown;
            while ( my $cell = $row->next_cell() ) {
                $rown     = $cell->row();
                my $col   = $cell->col();
                my $value = $cell->value();
                
                next if !defined($value);
                $value =~ s/^\s+//;
                $value =~ s/\s+$//;
                
                $row[$col] = $value;
            }
            # Взлет выявляется как:
            # 1 - строка заголовка = "заполнен только второй столбец и возможно третий"
            # 2 - заголовок таблицы = "первый столбец - №" - единственный уникальный формат, поэтому к нему привязываемся жестко
            # 3 - персоны = первый столбец - цифра
            # 4 - спец-персоны = заполнен третий стоблец
            # 5 - когда заканчиваются спец-персоны, то заканчивается и взлет
            
            if ($row[0] && ($row[0] eq '№') && ($row[1] eq 'Ф. И. О.')) {
                # Этап 2.
                # заголовок таблицы = "первый столбец - №"
                # единственный уникальный формат, поэтому к нему привязываемся жестко
                # т.е. при попадании на эту строку принудительно передергиваем взлет
                
                # Формируем новый взлет (появился заголовок таблицы)
                $prev_title ||= ['', '', ''];
                $fly = { name => $prev_title->[1], meta => $prev_title->[2]||'', pers => [] };
                # Привязка к странице
                $fly->{sheetid} = $flySheet->{id} if $flySheet;
                if ($prev_title->[1] =~ /пример взлет/) {
                    $fly->{isexample} = 1;
                }
                else {
                    $fnum ++;
                    $fly->{flyid} = $fly->{sheetid} . '-' . $fnum if $fly->{sheetid};
                    push @$flylist, $fly;
                }
                undef $prev_spec;
                next;
            }

            # Этап 1?
            # Все, что потенциально м.б. заголовком
            # Однозначно идентифицировать заголовок мы можем не по первой строке (где название), а по второй.
            # Поэтому все, что потенциально м.б. похоже на первую строчку - временно сохраняем до следующей итерации
            # В новом формате шаблона в первом и пятом столбцах всегда что-то есть, поэтому
            # на них не ориентируемся, но несколько следующих должны быть пустыми
            $prev_title = (!$row[0] || ($row[0] eq '-')) && !$row[3] && !$row[5] && !$row[6] && $row[1] ? [@row] : undef;
            
            # т.к. мы совершенно точно идентифицируем заголовок взлета, все дальнейшее отсчитываем только от него
            $fly || next;
            
            # Этап 3 или 4
            # по 3-му столбцу определяем, является ли строка спец-персоной
            my ($spec) = grep { $row[2] && $_->{regexp} && ($row[2] =~ /$_->{regexp}/) } @{ $flySheet->{hdr}||[] };
            # формируем персону
            my $pers =  {
                    name        => $row[1],
                    code        => $row[2],
                    parashute   => $row[3],
                    visota      => $row[4],
                    op          => $row[5],
                    zp          => $row[6],
                    prygcount   => $row[7],
                    zadanie     => $row[8],
                    cashout     => $row[9],
                };
            foreach my $f (keys %$pers) {
                $pers->{$f} = '' if !defined($pers->{$f}) || ($pers->{$f} eq '-');
            }
            
            if (!$prev_spec && !$spec && $row[0] && ($row[0] =~ /^\d+$/)) {
                # Этап 3.
                # Еще не было до этого спец-персон, эта строчка тоже таковой не является
                # и обязательно есть номер в первом столбце
                
                # Тут забиваем все пустые места, лишних отфильтруем потом
                push @{ $fly->{pers} ||= [] }, $pers;
                
                # Костыль для глюка екселя в отношении чисел с плавающей точкой
                # относится только к номерам ОП вида NN,NN
                if ($pers->{op}) {
                    $pers->{op} =~ s/(\d\d)\.(\d\d)(0{10,}1|9{10,})?$/$1,$2/;
                }
            }
            
            elsif ($spec && $fly->{pers} && @{ $fly->{pers} }) {
                # Этап 4.
                # Пошли спецперсоны
                # Но они могут пойти только после обычных персон
                push @{ $fly->{spec} ||= [] }, $pers;
                
                $prev_spec = $spec;
            }
            
            elsif ($prev_last || ($prev_spec && $fly->{pers} && @{ $fly->{pers} } && (!$row[0] || $row[0] eq '-'))) {
                # Этап 5.
                # Спец-персоны кончились - кончился весь взлет
                # Но при этом обязательно должны были быть и обычные персоны и спец
                # Так же к этому моменту первый столбец д.б. уже пустой
                undef $fly;
                undef $prev_spec;
                next;
            }
            
            elsif (!$fly->{isexample}) {
                # Какая-то непонятная ошибка парсинга
                foreach (@row) { $_ = '' if !defined($_) }
                push @{ $fly->{error} ||= [] },
                    sprintf("Unknown row[%d]: %s", $rown+1, join(', ', @row));
                error('Unknown row[%d:%d]: %s', $sheetno+1, $rown+1, join(', ', @row));
                next;
            }
            
            $prev_last = $row[0] && ($row[0] eq '#') ? 1 : 0;
        }
        
        $sheetno++;
    }
    
    # Грепаем все взлеты, где нет ни одной персоны
    @$flylist =
        grep {
            my $fly = $_;
            while (my $pers = $fly->{pers}->[-1]) {
                my @fields =
                    grep { $pers->{$_} }
                    qw/name parashute op zp prygcount zadanie cashout/;
                if (@fields) {
                    last;
                }
                else {
                    pop @{ $fly->{pers} };
                }
            }
            
            @{ $fly->{pers} } ? 1 : 0;
        }
        @$flylist;
    
    # Расход
    if ($use{ballance} && ($sheetno = c('sheetBalance'))) {
        my $ballp = ($r{ballance} = []);
        my $worksheet = $workbook->worksheet($sheetno-1);
        
        my $type = undef;
        while ( my $row = $worksheet->next_row() ) {
            my @row = ();
            my $rown;
            while ( my $cell = $row->next_cell() ) {
                $rown     = $cell->row();
                my $col   = $cell->col();
                my $value = $cell->value();
                
                next if !defined($value);
                $value =~ s/^\s+//;
                $value =~ s/\s+$//;
                
                $row[$col] = $value; #{ map { ($_ => $cell->{$_}) } grep { !ref($cell->{$_}) } keys %$cell };
            }
        
            if ($row[0] && $row[1] && ($row[1] =~ /^кол/i)) {
                $type = { name => $row[0], count => $row[2] };
                push @$ballp, $type;
            }
            elsif ($type && $row[1] && ($row[1] =~ /^сумм/i)) {
                $type->{summ} = $row[2];
            }
            else {
                undef $type;
            }
        }
    }
    
    # Постоянный состав
    if ($use{people} && ($sheetno = c('sheetPeople'))) {
        my $people = ($r{people} = {});
        my $worksheet = $workbook->worksheet($sheetno-1);
        my @head = ();
        
        while ( my $row = $worksheet->next_row() ) {
            my @row = ();
            my $rown;
            while ( my $cell = $row->next_cell() ) {
                $rown     = $cell->row();
                my $col   = $cell->col();
                my $value = $cell->value();
                
                next if !defined($value);
                $value =~ s/^\s+//;
                $value =~ s/\s+$//;
                
                $row[$col] = $value;
            }
            
            if ($row[0] && $row[1] && ($row[0] eq '№') && ($row[1] eq 'ФИО')) {
                @head = map {
                        my $val = $_;
                        my ($f) = grep { $val =~ /$_->{regexp}/i } @::PeopleColumn;
                        $f ? $f->{code} : $val;
                    } @row;
                next;
            }
            
            next if (@head < 3) || (@row < 3) || !$row[0] || ($row[0] !~ /^\d+$/) || !$row[1];
            
            my $p = ($people->{$row[1]} = {
                    row     => $rown,
                    num     => $row[0],
                    name    => $row[1],
                });
            
            foreach my $n (2 .. (scalar(@row)-1)) {
                defined($row[$n]) || next;
                $p->{$head[$n]||$n} = $row[$n];
            }
        }
    }
    
    # Завершаем
    undef $workbook;
    undef $reader;
    if (-e $tmpdir) {
        debug('remove files from %s', $tmpdir);
        system('rm -rf '.$tmpdir);
    }
    undef $error;
    
    # Возвращаем нужные данные
    return
        wantarray ?
            (map { $r{$_} } @_) :
        @_ ?
            $r{shift()} :
            ();
}

####################################################

1;
