# Полный процесс настройки сервера `Монитора Взлётов`

Протестировано на:

* Железо: Raspberry PI 4 model B 2 GB
* ОС: FreeBSD 13.0
* Подключение сети через: Ethernet
* SD-карта: SanDisk Ultra 32 GB

Подключение по `WiFi` не поддерживается этой `ОС` на данной аппаратной платформе.

## Установка ОС на SD-карту

Скачиваем образ с сайта [freebsd](https://download.freebsd.org/ftp/releases/arm64/aarch64/ISO-IMAGES/).

Подходит тот, в названии которого в конце есть RPI. Например:

    FreeBSD-13.0-RELEASE-arm64-aarch64-RPI.img.xz

Запускаем утилиту [balena Etcher](https://www.balena.io/etcher/) и записываем этот образ на SD-карту.

Вставляем SD-карту в Raspberry, подключенный к монитору, включаем.

Стандартный логин/пароль: root/root.

Устанавливаем пакеты:

    pkg install bash mc

Соглашаемся везде: `y` и `enter`.

У пользователя `root` правим настройки:

    pw usermod root -s /usr/local/bin/bash -c root

Добавляем пользователя:

<pre>
[root@generic ~]# 📝 <b>adduser</b>
Username: 📝 <b>cliff</b>
Full name: 📝 <b>Cliff</b>
Uid (Leave empty for default):
Login group [cliff]: 📝 <b>wheel</b>
Login group is wheel. Invite cliff into other groups? []:
Login class [default]:
Shell (sh csh tcsh bash rbash nologin) [sh]: 📝 <b>bash</b>
Home directory [/home/cliff]:
Home directory permissions (Leave empty for default):
Use password-based authentication? [yes]:
Use an empty password? (yes/no) [no]:
Use a random password? (yes/no) [no]:
Enter password: 📝 <b>пароль</b>
Enter password again: 📝 <b>пароль</b>
Lock out the account after creation? [no]:
Username   : cliff
Password   : *****
Full Name  : Cliff
Uid        : 1002
Class      :
Groups     : wheel
Home       : /home/cliff
Home Mode  :
Shell      : /usr/local/bin/bash
Locked     : no
OK? (yes/no): 📝 <b>yes</b>
adduser: INFO: Successfully added (cliff) to the user database.
Add another user? (yes/no): 📝 <b>no</b>
Goodbye!
[root@generic ~]#
</pre>

Удаляем дефолтного пользователя:

<pre>
[root@generic ~]# 📝 <b>rmuser freebsd</b>
Matching password entry:

freebsd:$6$RaLQ0fcavdJkKhd8$gjhyY9Ech27BcxYa/rn3oLdgA/PC14W4mNi9OZDrhjHAYoiuV87uKQ/Hf6MXxO7dgC52PDjd9omdf1dlZFY8J.:1001:1001::0:0:FreeBSD User:/home/freebsd:/bin/csh

Is this the entry you wish to remove? 📝 <b>y</b>
Remove user's home directory (/home/freebsd)? 📝 <b>y</b>
Removing user (freebsd): mailspool home passwd.
[root@generic ~]#
</pre>

Посмотреть текущий полученный IP:

<pre>
[root@generic ~]# 📝 <b>ifconfig genet0</b>

genet0: flags=8843<UP,BROADCAST,RUNNING,SIMPLEX,MULTICAST> metric 0 mtu 1500
	options=68000b<RXCSUM,TXCSUM,VLAN_MTU,LINKSTATE,RXCSUM_IPV6,TXCSUM_IPV6>
	ether dc:a6:32:0e:f9:10
	inet <b>192.168.3.159</b> netmask 0xffffff00 broadcast 192.168.3.255
	media: Ethernet autoselect (100baseTX <full-duplex>)
	status: active
	nd6 options=29<PERFORMNUD,IFDISABLED,AUTO_LINKLOCAL>
</pre>


## Способы отредактировать файлы

В тексте этого документа будут встречаться настройки, которые надо изменить в определённых файлах.

* `ee имя-файла`

    Этот редактор есть в системе по умолчанию. Он консольный.
    
    В этом редакторе, чтобы попасть в меню, надо нажать ESC и подождать. 
    Следом нажимаем два раза Enter - это пункты: `выйти` и `сохранить изменения`.
    
    Большинство необходимых комбинаций клавиш там указано в самом верху. `^` - это клавиша `ctrl`.

* `mc`

    Это Midnight Commander. Те, кто хоть раз пользовался Norton Commander, без труда разберутся.

* `cat > имя-файла`

    Этот способ удобен для ситуаций, где указан весь файл целиком.
    
    ⚠ Эта команда полностью стирает предыдущее содержимое файла!!!
    
    После ввода команды - вставляем содержимое файла и жмём `ctrl` + `D`

## Базовая настройка системы

Подключаемся по `SSH` на указанный `IP`, авторизуясь по ранее добавленному пользователю.

Все изменения делаем под пользователем `root` (пароль по умолчанию: `root`):

<pre>
[cliff@generic ~]$ 📝 <b>su</b>
Password:📝 <b>пароль-для-root</b>
[root@generic /home/cliff]#
</pre>

Редактируем:

    ee /etc/ssh/sshd_config

Правим параметры (найти в файле и изменить):

    Port 9122
    
    PrintMotd no

Дописываем в конец:

    PubkeyAcceptedKeyTypes=+ssh-dss

Выполняем:

    ln -s /etc/rc.d /rc.d
    /rc.d/sshd reload

>⚠ **Обратите внимание**
>
>Мы только что сменили порт `SSH` со стандартного на `9122`, поэтому его надо будет указать при следующем подключении.

Правим локальный timezonê:

    tzsetup

Отвечаем:

    Is this machine's CMOS clock set to UTC?
    
    No -> Europe -> Russian Federation -> MSK+00 - Moscow area

На вопрос:

    Does the abbreviation `MSK' look reasonable?
    
    Yes

## Обновление безопасности ОС

Выполняем скачивание обновлений:

    freebsd-update fetch

В конце выполнения скрипт показывает, какие файлы будут изменены.
Несколько раз нажимаем `q`. Теперь запускаем установку:
    
    freebsd-update install

Перезагружаемся:

    reboot

## Донастройка ОС

### Устанавливаем пакеты

    pkg install nginx memcached redis git-lite mpd5 \
        p5-Module-Find p5-Archive-Zip p5-OLE-Storage_Lite p5-XML-LibXML \
        p5-Cache-Memcached-Fast p5-RedisDB p5-JSON-XS \
        p5-WWW-Telegram-BotAPI \
        p5-FCGI p5-FCGI-ProcManager

Соглашаемся везде: `y` и `enter`

### Ещё немного команд

    ln -s /usr/local/bin/perl /usr/bin/perl
    
    mkdir /var/run/memcached
    chown nobody:nobody /var/run/memcached
    
    mkdir /home/manifest.tmp
    mkdir /home/redis
    chown redis:redis /home/redis

### /etc/crontab

Дописываем в самый конец:

    */30    *       *       *       *       root    /usr/sbin/ntpdate -u 0.ru.pool.ntp.org 1.ru.pool.ntp.org 2.ru.pool.ntp.org >> /dev/null
    */1     *       *       *       *       root    /home/manifest/bin/flyevent
    4       8       *       *       *       root    /home/manifest/bin/oldchat
    2       23      *       *       *       root    /bin/rm /home/manifest.tmp/ppu.txt
    3       1       *       *       *       root    /usr/local/etc/rc.d/mpd5 restart

### /etc/fstab

Дописываем в самый конец:

    md0 /home/manifest.tmp mfs rw,nosuid,noexec,-s128M 0 0

Это создаст раздел для временных файлов, хранимый в RAM

### /etc/rc.conf

Файл целиком:

    hostname="manifest.cliffa.net"
    
    ifconfig_DEFAULT="DHCP"
    ifconfig_genet0_alias0="inet 10.0.0.240/24"
    ifconfig_genet0_alias1="inet 1.1.1.240/32"
    
    sshd_enable="YES"
    mpd_enable="YES"
    
    sendmail_enable="NONE"
    sendmail_submit_enable="NO"
    sendmail_outbound_enable="NO"
    sendmail_msp_queue_enable="NO"
    
    growfs_enable="YES"
    
    memcached_enable="YES"
    memcached_flags="-a 0777 -s /var/run/memcached/memcached.sock"
    redis_enable="YES"
    
    nginx_enable="YES"
    manifest_main_enable="YES"
    manifest_telegramd_enable="YES"

### /usr/local/etc/nginx/nginx.conf

Файл целиком:

    #user  nobody;
    worker_processes  10;
    
    events {
        worker_connections  1024;
    }
    
    http {
        include       mime.types;
        default_type  application/octet-stream;
        
        sendfile        on;
        #tcp_nopush     on;
        
        #keepalive_timeout  0;
        keepalive_timeout  65;
        
        #gzip  on;
        
        server {
            listen       80;
            server_name  monitor.my;
            
            #access_log  /var/log/manifest/nginx.access.log;
            error_log  /var/log/manifest/nginx.error.log;
            
            location / {
                include fastcgi_params;
                fastcgi_pass   127.0.0.1:9091;
                fastcgi_buffers 16 32k;
                fastcgi_buffer_size 64k;
                client_max_body_size 15m;
                fastcgi_request_buffering off;
            }
            
            location ~ "\.[a-zA-Z0-9]{2,4}$" {
                root /home/manifest/html;
            }
            
            error_page 404 500 502 503 504 /error.html;
        }
    }

### /usr/local/etc/redis.conf

Найти и изменить строчку (примерно №454):

    dir /home/redis/

### /usr/local/etc/mpd5/mpd.conf

Файл целиком:

    startup:
    	# configure mpd users
    	set user foo bar admin
    	set user foo1 bar1
    	# configure the console
    	set console self 127.0.0.1 5005
    	set console open
    	# configure the web server
    	set web self 0.0.0.0 5006
    	set web open
        
    default:
    	load northnet
        
    northnet:
    	create bundle static B1
    	#set iface route default
    	set iface route 10.190.0.0/16
    	set iface route 91.219.24.0/22
    	set iface route 192.168.3.0/24
    	set iface route 192.168.50.0/24
    	set iface route 192.168.64.0/21
    	set iface up-script /usr/local/etc/mpd5/c.up
        
    	create link static L1 pptp
    	set link action bundle B1
    	set auth authname <пользователь>
    	set auth password <пароль>
    	set link max-redial 0
    	set link redial-delay 20
    	set link mtu 1400
    	set link keep-alive 20 75
    	set pptp peer <host>
    	set pptp disable windowing
        
    	open

### /usr/local/etc/mpd5/c.up

Файл целиком (надо создать):

    #!/bin/sh
    # system: command "/usr/local/etc/mpd5/c.up ng0 inet 10.190.2.100/32 192.168.5.1 '-' '' '' '91.219.25.27'"
    wanip=`/sbin/route -n get default | sed -rn 's/^ +gateway: +(.*)/\1/p'`
    echo $wanip > /home/.defaultgateway
    /sbin/route delete $8
    /sbin/route add $8 $wanip

Выполнить:

    chmod a+x /usr/local/etc/mpd5/c.up

## Установка ПО-манифест

Единственный модуль, который не получается установить через менеджер пакетов `pkg`, будем ставить через установщик perl-пакетов `cpan`.

Выполняем:

    cpan

На вопрос:

    Would you like to configure as much as possible automatically? [yes]

Жмём `enter`.

При появлении приглашения:

    cpan[1]>

Пишем:

    install URI::redis

Жмём `enter` и довольно долго ждём, пока установятся все зависимости и нужный нам модуль. Выход из `cpan` с помощью `ctrl` + `D`.

И у меня не получилось, почему-то, с первого раза. Установщик завершился с таким текстом:

    Writing Makefile for URI::redis
    Writing MYMETA.yml and MYMETA.json
    ==> Your Makefile has been rebuilt. <==
    ==> Please rerun the make command.  <==
    false
    *** Error code 1

    Stop.
    make: stopped in /root/.cpan/build/URI-redis-0.02-0
      MENDEL/URI-redis-0.02.tar.gz
      /usr/bin/make -- NOT OK
    Failed during this command:
     RSHERER/Class-Data-Inheritable-0.09.tar.gz   : make NO
     DROLSKY/Devel-StackTrace-2.04.tar.gz         : make NO
     DROLSKY/Exception-Class-1.45.tar.gz          : make NO
     RJBS/Test-Deep-1.130.tar.gz                  : make NO
     DAGOLDEN/Capture-Tiny-0.48.tar.gz            : make NO
     RJBS/Algorithm-Diff-1.201.tar.gz             : make NO
     NEILB/Text-Diff-1.45.tar.gz                  : make NO
     DCANTRELL/Test-Differences-0.69.tar.gz       : make NO
     DAGOLDEN/Sub-Uplevel-0.2800.tar.gz           : make NO
     EXODIST/Test-Exception-0.43.tar.gz           : make NO
     BIGJ/Test-Warn-0.36.tar.gz                   : make NO
     OVID/Test-Most-0.37.tar.gz                   : make NO
     MENDEL/URI-redis-0.02.tar.gz                 : make NO

Я вышел из cpan (`ctrl` + `D`), выполнил:

    rm -rf /root/.cpan/build/URI-redis-0.02-0

Снова зашел в cpan и выполнил:

    install URI::redis

Ещё долго ждал (установилось ещё какое-то количество модулей, и долго выполнялись разные тесты) и... всё удалось!

Вот так выглядит в конце (возможно, Вы увидите это сразу без необходимости повторно выполнять установку модуля):

    Installing /usr/local/lib/perl5/site_perl/URI/redis_Punix.pm
    Installing /usr/local/lib/perl5/site_perl/URI/redis.pm
    Installing /usr/local/lib/perl5/site_perl/man/man3/URI::redis.3
    Installing /usr/local/lib/perl5/site_perl/man/man3/URI::redis_Punix.3
    Appending installation info to /usr/local/lib/perl5/5.32/mach/perllocal.pod
      MENDEL/URI-redis-0.02.tar.gz
      /usr/bin/make install  -- OK

    cpan[2]>

Не забываем выйти из `cpan` с помощью `ctrl` + `D`.

Теперь выполняем:

    mkdir /usr/local/lib/perl5/site_perl/Clib
    ln -s /home/Clib/Proc.pm /usr/local/lib/perl5/site_perl/Clib/Proc.pm
    ln -s /home/manifest/rc.d/freebsd/manifest.main.fcgi /usr/local/etc/rc.d/manifest.main.fcgi
    ln -s /home/manifest/rc.d/freebsd/manifest.telegramd /usr/local/etc/rc.d/manifest.telegramd
    
    mkdir /home/manifest
    
    mkdir /var/log/manifest
    chmod a+w /var/log/manifest
    mkdir /var/run/manifest
    chmod a+w /var/run/manifest
    
    fetch -o /home/manifest/files.local.tar https://github.com/cliffanet/manifest/raw/master/files.local.tar
    tar -xf /home/manifest/files.local.tar -C /home/manifest/
    
    mkdir /home/manifest/lib
    ln -s /home/Clib /home/manifest/lib/Clib
    
    cd /home/manifest
    ./export.master.sh


## Завершение

В самом конце обязательно делаем [Процедуры по восстановлению с резервной SD-карты](restore.md).
