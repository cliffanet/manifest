package xls2fly;

use strict;
use warnings;
use utf8;

use Clib::Const;
use Clib::Log;

use Excel::Reader::XLSX;

#######################################################################
#       В этом модуле представлен функционал для формирования
#       данных до их попадания во временную БД (MemCached)
#######################################################################

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

####################################################
##
##  Базовый парсинг исходного файла
##
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
                        my ($f) = grep { $val =~ /$_->{regexp}/i } @{ c('PeopleColumn') || [] };
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
##
##  Формирование дополнительных полей на основании существующих данных
##
sub adding_data {
    shift() if $_[0] && (($_[0] eq __PACKAGE__) || (ref($_[0]) eq __PACKAGE__));
    my $log = log_prefix('xls2fly->adding_data()');
    
    #my $flyempty = 0;
    #my $prevsheet = '';
    foreach my $fly (@_) {
        my ($flySheet) = grep { $_->{id} eq $fly->{sheetid} } @{ c('flySheet') || [] };
        
        $fly->{meta} = 'x' if $fly->{name} =~ /подъем/;
        
        #
        # Персоны
        #
        foreach my $pers ( @{ $fly->{pers}||=[] } ) {
            # Платность
            $pers->{payed} = 1
                if $pers->{code} && (
                        ($pers->{code} =~ /^([Дд][Оо][Лл][Гг]\s+)?\d+$/) ||
                        ($pers->{code} =~ /безнал/i) ||
                        ($pers->{code} =~ /сс/i) ||
                        ($pers->{code} =~ /cc/i) ||
                        ($pers->{code} =~ /^[Мм][Пп][Ии]/i) ||
                        ($pers->{code} =~ /^[Оо][Пп][Ее][Рр]/i)
                    );
        }
        
        #
        # Новый способ группировки
        #
        _pers_group(@{ $fly->{pers} });
        
        #
        # Тип конкретного посадочного места (после группировки, чтобы учитывать ее)
        #
        foreach my $pers ( @{ $fly->{pers} } ) {
            foreach my $t (@{ c('persType') || [] }) {
                my @regexp = grep { /^regexp\d*_[a-z\d\_]+/i } keys %$t;
                # фильтр по полям, их м.б. несколько для одной персоны с условием "и"
                my $ok = 1;
                foreach my $r (@regexp) {
                    if ($r =~ /regexp_fly_([a-z\d]+)$/i) {
                        my $f = $1;
                        my $patt = $t->{$r};
                        next if $f && defined($fly->{$f}) && ($fly->{$f} =~ /$patt/);
                    }
                    elsif ($r =~ /regexp_([a-z\d]+)$/i) {
                        my $f = $1;
                        my $patt = $t->{$r};
                        next if $f && defined($pers->{$f}) && ($pers->{$f} =~ /$patt/);
                    }
                    $ok = undef;
                    last;
                }
                $ok || next;
                
                next if $t->{ingroup} && (!$pers->{group} || !$pers->{grpind} || ($t->{ingroup} != $pers->{grpind}));
                
                $pers->{type} = $t->{id};
                last;
            }
            
            delete $pers->{grpind};
        }
        
        # Отфильтровываем пустые места:
        @{ $fly->{pers} } =
            grep {
                ($_->{name} ne '') || ($_->{group}) ||
                ($_->{visota} && $_->{parashute})
            }
            @{ $fly->{pers} };
    }
    
    1;
}

####################################################
##
##  Группировка персон во взлёте
##
sub _pers_group {
    shift() if $_[0] && (($_[0] eq __PACKAGE__) || (ref($_[0]) eq __PACKAGE__));
    my $log = log_prefix('xls2fly->_pers_group_old()');
    
    my @plist = @_;
    
    my @group = ();
    my %group = ();
    #
    while (my $pers = shift @plist) {
        #
        push @group,
            map { {
                %$_,
                flist => [@{$_->{list}}], # Список фильтров, будем отсюда вырезать строки
                plist => [],        # Список вошедших персон
            } }
            grep { ref($_->{list}) eq 'ARRAY' }
            @{ c('persGroup') || [] };
        # Отбираем список фильтров, удовлетворяющих текущей персоне
        @group =
            grep {
                my $ret = 0;
                my $g = $_;
                while (my $f = shift @{ $g->{flist} }) {
                    if ($f->{any}) {
                        $ret = 1;
                        push @{ $g->{plist} }, $pers;
                    }
                    else {
                        my @regexp = grep { /^regexp\d*_[a-z\d]+/i } keys %$f;
                        if (@regexp) {
                            $ret = 1;
                            foreach my $r (@regexp) {
                                my $fld = $r =~ /_([a-z\d\_]+)$/i ? $1 : '';
                                my $patt = $f->{$r};
                                # Проверка полей
                                next if $fld && defined($pers->{$fld}) && ($pers->{$fld} =~ /$patt/);
                                
                                $ret = 0;
                                last;
                            }
                        }
                        if ($ret) {
                            push @{ $g->{plist} }, $pers;
                        }
                    }
                    
                    if (!$ret && $f->{optional}) {
                        # Фильтр не совпал, но он не обязательный
                        if (@{ $g->{flist} }) {
                            next; # Поэтому, если еще остались другие требуемые персоны-фильтры, то проверяем их
                        }
                        else {
                            # А если не осталось, тогда группа заполнена всем необходимым и ее надо обработать
                            $ret = 1;
                        }
                    }
                    last; # А если текущий фильтр совпал или не совпал и он обязательный, прерываем дальнейшую проверку по этому фильтру
                }
                
                $ret;
            }
            @group;
        
        # Смотрим первую группу, которая удовлетворяет всем параметрам
        my $islast = @plist ? 0 : 1;
        my ($g) =
            grep {
                my $gr = $_;
                (@{$gr->{plist}} > 1) &&                 # Проверяем наличие персон в группе
                !(grep {$_->{group}} @{$gr->{plist}}) && # но не должно быть ни одной, уже участвующей в другой группе
                (
                    !@{$gr->{flist}} ||                  # и не осталось элементов фильтра
                                                        # или это крайняя персона и не осталось обязательных элементов фильтра
                    ($islast && !(grep {!$_->{optional}} @{$gr->{flist}}))
                )
            }
            @group;
        if ($g) {
            # Если есть совпадение, присваиваем группу
            my $groupid;
            foreach my $n (1..99) {
                # подбираем groupid, свободный в этом взлете
                $groupid = sprintf "%s-%02d", $g->{id}, $n;
                last if !$group{$groupid};
            }
            $group{$groupid} = 1;
            my $n = 0;
            # Присваиваем группу всем персонам
            foreach my $pers1 (@{$g->{plist}}) {
                $pers1->{group} = $groupid;
                $pers1->{grpind} = ++$n;
            }
        }
        
        # Оставляем только группы, где остались фильтры
        @group = grep { @{$_->{flist}} && @{$_->{plist}} } @group;
    }
    
    1;
}
sub _pers_group_old {
    shift() if $_[0] && (($_[0] eq __PACKAGE__) || (ref($_[0]) eq __PACKAGE__));
    my $log = log_prefix('xls2fly->_pers_group_old()');

    # Параметры, определяющие текущую группу участников
    my %group = ();
    my $group = undef;
    my $groupid = undef;
    my $groupfirst = undef;
    my $ingroup = undef;
    #
    foreach my $pers ( @_ ) {
        foreach my $g (@{ c('persGroup') || [] }) {
            my @regexp = grep { /^regexp\d*_[a-z\d]+/i } keys %$g;
            @regexp || next;
            my $ok = 1;
            foreach my $r (@regexp) {
                my $f = $r =~ /_([a-z\d\_]+)$/i ? $1 : '';
                my $patt = $g->{$r};
                # Проверка полей
                next if $f && defined($pers->{$f}) && ($pers->{$f} =~ /$patt/);
                ## Проверка на правило "no_group"
                #next if ($f eq 'no_group') && (!$groupid || ($groupid !~ /$patt/));
                
                $ok = 0;
                last;
            }
            $ok || next;
            # Очередная строчка совпадает с первым членом группы
            $group = $g;
            $groupfirst = $pers;
            foreach my $n (1..99) {
                # подбираем groupid, свободный в этом взлете
                $groupid = sprintf "%s-%02d", $g->{id}, $n;
                last if !$group{$groupid};
            }
            # Формируем аттрибутов персон, которые должны\могут войти в группу
            # первым в списке указываем любого - это текущая персона
            # массив хешей "after" пересобираем, т.к. нам его надо модифицировать
            $group{$groupid} = [ { any => 1 }, map { {%$_} } @{ $g->{after}||[] } ];
            $ingroup = 0;
            last;
        }
        if ($group && $groupid && (my $after = $group{$groupid})) {
            my $attr;
            foreach my $attr1 (@$after) {
                if ($attr1->{any}) {
                    # Первым проверяем аттрибут any
                    $attr = $attr1;
                }
                elsif (my @regexp = grep { /^regexp\d*_[a-z\d]+/i } keys %$attr1) {
                    # фильтр по полям, их м.б. несколько для одной персоны с условием "и"
                    $attr = $attr1;
                    foreach my $r (@regexp) {
                        my $f = $r =~ /regexp_([a-z\d]+)$/i ? $1 : '';
                        my $patt = $attr1->{$r};
                        next if $f && defined($pers->{$f}) && ($pers->{$f} =~ /$patt/);
                        $attr = undef;
                        last;
                    }
                }
                last if $attr;
            }
            if ($attr) {
                $ingroup ++;
                # Убираем из списка исползованный элемент
                @$after = grep { $_ ne $attr } @$after;
                if ($groupfirst && ($groupfirst ne $group)) {
                    # Это не первый элемент
                    $pers->{payed} = 1
                        if $groupfirst->{payed};
                }
                
            }
            else {
                # Кончились допустимые элементы в группе
                $group = undef;
                $groupid = undef;
                $groupfirst = undef;
                $ingroup = undef;
            }
        }
        
        if ($group && $groupid) {
            $pers->{group} = $groupid;
            $pers->{grpind} = $ingroup;
        }
    }
    
    1;
}


####################################################
##
##  Сравнение свежезагруженного списка взлетов с предыдущей версией
##  и вычисление необходимых доп параметров
##
sub by_prev {
    shift() if $_[0] && (($_[0] eq __PACKAGE__) || (ref($_[0]) eq __PACKAGE__));
    my $log = log_prefix('xls2fly->_pers_group_old()');
    
    my $flist = shift() || return;
    my $fprev = shift() || return;
    
    my %fold =
        map { $_->{flyid} ? ($_->{flyid} => $_) : () }
        @$fprev;
    
    # Парсим новый список
    my $time = time();
    foreach my $fly (@$flist) {
        # Ищем предыдущий вариант этого же взлета
        # Должен совпадать порядковый номер в списке всех взлетов
        $fly->{flyid} || next;
        my $fold = $fold{ $fly->{flyid} };
        # И так же - название. Смена название принимается за изменение порядка в списке
        # и тогда найти предыдущий скорее всего не получится
        if (!$fold || !$fold->{name} || !$fly->{name} || ($fold->{name} ne $fly->{name})) {
            # При любой из этих проблем помечаем meta как fail
            $fly->{meta_fail} = 1 if defined($fly->{meta}) && ($fly->{meta} ne '');
            next;
        }
        
        # Фисксируем момент изменение "meta"
        $fly->{meta} = '' if !defined($fly->{meta});
        my $mprev = defined($fold->{meta}) ? $fold->{meta} : '';
        if ($fly->{meta} ne $mprev) {
            # При изменении meta отмечаем это параметрами meta_prev и meta_time
            $fly->{meta_prev} = $mprev;
            $fly->{meta_prtm} = $fly->{meta_time} if defined $fly->{meta_time};
            $fly->{meta_time} = $time;
        }
        else {
            # Если ничего не менялось,
            # сохраняем параметры meta_prev и meta_time между обновлениями файла
            foreach my $f (qw/meta_prev meta_prtm meta_time meta_fail/) {
                $fly->{$f} = $fold->{$f} if exists $fold->{$f};
            }
        }
    }
    
    1;
}

####################################################

1;
