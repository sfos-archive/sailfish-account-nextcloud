Name: sailfish-account-nextcloud
License: Proprietary
Version: 0.0.1
Release: 1
Source0: %{name}-%{version}.tar.bz2
Summary: Account plugin for Nextcloud
Group:   System/Libraries
BuildRequires: qt5-qmake

%description
%{summary}.

%files
%defattr(-,root,root,-)
%{_datadir}/accounts/providers/nextcloud.provider
%{_datadir}/accounts/services/nextcloud-backup.service
%{_datadir}/accounts/services/nextcloud-caldav.service
%{_datadir}/accounts/services/nextcloud-carddav.service
%{_datadir}/accounts/services/nextcloud-images.service
%{_datadir}/accounts/services/nextcloud-sharing.service
%{_datadir}/accounts/ui/nextcloud.qml
%{_datadir}/accounts/ui/nextcloud-settings.qml
%{_datadir}/accounts/ui/nextcloud-update.qml


%package -n buteo-sync-plugin-nextcloud-backup
Summary:   Provides synchronisation of backup/restore blobs with Nextcloud
Group:     System/Applications
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(mlite5)
BuildRequires: pkgconfig(buteosyncfw5)
BuildRequires: pkgconfig(libsignon-qt5)
BuildRequires: pkgconfig(accounts-qt5)
BuildRequires: pkgconfig(socialcache)
BuildRequires: pkgconfig(libsailfishkeyprovider)
Requires: %{name} = %{version}-%{release}
Requires: buteo-syncfw-qt5-msyncd
Requires: systemd
Requires(post): systemd

%description -n buteo-sync-plugin-nextcloud-backup
Provides synchronisation of backup/restore blobs with Nextcloud.

%files -n buteo-sync-plugin-nextcloud-backup
%defattr(-,root,root,-)
#out-of-process-plugin form:
/usr/lib/buteo-plugins-qt5/oopp/nextcloud-backup-client
#in-process-plugin form:
#/usr/lib/buteo-plugins-qt5/libnextcloud-backup-client.so
%config %{_sysconfdir}/buteo/profiles/client/nextcloud-backup.xml
%config %{_sysconfdir}/buteo/profiles/sync/nextcloud.Backup.xml


%package -n buteo-sync-plugin-nextcloud-images
Summary:   Provides synchronisation of gallery images with Nextcloud
Group:     System/Applications
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(mlite5)
BuildRequires: pkgconfig(buteosyncfw5)
BuildRequires: pkgconfig(libsignon-qt5)
BuildRequires: pkgconfig(accounts-qt5)
BuildRequires: pkgconfig(socialcache)
BuildRequires: pkgconfig(libsailfishkeyprovider)
Requires: %{name} = %{version}-%{release}
Requires: buteo-syncfw-qt5-msyncd
Requires: systemd
Requires(post): systemd

%description -n buteo-sync-plugin-nextcloud-images
Provides synchronisation of gallery images with Nextcloud.

%files -n buteo-sync-plugin-nextcloud-images
%defattr(-,root,root,-)
#out-of-process-plugin form:
/usr/lib/buteo-plugins-qt5/oopp/nextcloud-images-client
#in-process-plugin form:
#/usr/lib/buteo-plugins-qt5/libnextcloud-images-client.so
%config %{_sysconfdir}/buteo/profiles/client/nextcloud-images.xml
%config %{_sysconfdir}/buteo/profiles/sync/nextcloud.Images.xml


%package -n transferengine-plugin-nextcloud
Summary: Nextcloud file sharing plugin for Transfer Engine
Group: System/Libraries
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(mlite5)
BuildRequires: pkgconfig(buteosyncfw5)
BuildRequires: pkgconfig(libsignon-qt5)
BuildRequires: pkgconfig(accounts-qt5)
BuildRequires: pkgconfig(socialcache)
BuildRequires: pkgconfig(libsailfishkeyprovider)
BuildRequires: qt5-qttools
BuildRequires: qt5-qttools-linguist
Requires: sailfishsilica-qt5
Requires: declarative-transferengine-qt5
Requires: %{name} = %{version}-%{release}

%description -n transferengine-plugin-nextcloud
Nextcloud file sharing plugin for Transfer Engine.

%files -n transferengine-plugin-nextcloud
%defattr(-,root,root,-)
%{_libdir}/nemo-transferengine/plugins/libnextcloudshareplugin.so
%{_datadir}/nemo-transferengine/plugins/NextcloudShareImage.qml
%{_datadir}/translations/sailfish_transferengine_plugin_nextcloud_eng_en.qm


%package -n transferengine-plugin-nextcloud-ts-devel
Summary:  Translation source for Nextcloud Transfer Engine share plugin
Group:    System/Libraries
Requires: transferengine-plugin-nextcloud

%description -n transferengine-plugin-nextcloud-ts-devel
Translation source for Nextcloud Transfer Engine share plugin.

%files -n transferengine-plugin-nextcloud-ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/sailfish_transferengine_plugin_nextcloud.ts


%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 "VERSION=%{version}"
make

%install
%qmake5_install

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
