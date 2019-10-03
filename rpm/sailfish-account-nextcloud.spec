Name: sailfish-account-nextcloud
License: Proprietary
Version: 0.0.1
Release: 1
Source0: %{name}-%{version}.tar.bz2
Summary: Account plugin for Nextcloud
Group:   System/Libraries
BuildRequires: qt5-qmake
BuildRequires: sailfish-svg2png

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
# TODO: only install the appropriate scale factor icons for the build target?  How?
%{_datadir}/themes/sailfish-default/meegotouch/z1.0/icons/graphic-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.0/icons/graphic-m-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.0/icons/graphic-s-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.0/icons/icon-l-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.25/icons/graphic-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.25/icons/graphic-m-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.25/icons/graphic-s-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.25/icons/icon-l-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5/icons/graphic-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5/icons/graphic-m-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5/icons/graphic-s-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5/icons/icon-l-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5-large/icons/graphic-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5-large/icons/graphic-m-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5-large/icons/graphic-s-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5-large/icons/icon-l-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.75/icons/graphic-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.75/icons/graphic-m-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.75/icons/graphic-s-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.75/icons/icon-l-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z2.0/icons/graphic-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z2.0/icons/graphic-m-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z2.0/icons/graphic-s-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z2.0/icons/icon-l-nextcloud.png


%package -n buteo-sync-plugin-nextcloud-backup
Summary:   Provides synchronisation of backup/restore blobs with Nextcloud
Group:     System/Applications
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(Qt5SystemInfo)
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


%package -n jolla-gallery-extension-nextcloud
Summary:   Provides integration of Nextcloud images into Gallery application
Group:     System/Libraries

%description -n jolla-gallery-extension-nextcloud
Provides integration of Nextcloud images into Gallery application.

%files -n jolla-gallery-extension-nextcloud
%defattr(-,root,root,-)
%{_datadir}/translations/gallery-extension-nextcloud_eng_en.qm
%{_datadir}/jolla-gallery/mediasources/NextcloudCacheMediaSource.qml
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/NextcloudGalleryIcon.qml
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/NextcloudUsersPage.qml
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/NextcloudAlbumsPage.qml
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/NextcloudAlbumDelegate.qml
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/NextcloudPhotoGridPage.qml
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/NextcloudFullscreenPhotoPage.qml
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/qmldir
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/libjollagallerynextcloudplugin.so
%dir %{_libdir}/qt5/qml/com/jolla/gallery/nextcloud


%package -n jolla-gallery-extension-nextcloud-ts-devel
Summary:  Translation source for Gallery Nextcloud plugin
Group:    System/Libraries
Requires: jolla-gallery-extension-nextcloud

%description -n jolla-gallery-extension-nextcloud-ts-devel
Translation source for Gallery Nextcloud plugin.

%files -n jolla-gallery-extension-nextcloud-ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/gallery-extension-nextcloud.ts


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
%{_datadir}/nemo-transferengine/plugins/NextcloudShareDialog.qml
%{_datadir}/translations/sailfish_transferengine_plugin_nextcloud_eng_en.qm
%{_libdir}/qt5/qml/Sailfish/TransferEngine/Nextcloud/qmldir
%{_libdir}/qt5/qml/Sailfish/TransferEngine/Nextcloud/NextcloudShareFile.qml
%{_libdir}/qt5/qml/Sailfish/TransferEngine/Nextcloud/libsailfishtransferenginenextcloudplugin.so
%dir %{_libdir}/qt5/qml/Sailfish/TransferEngine/Nextcloud


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
%qmake5 "VERSION=%{version}" "DEFINES+=BUTEO_OUT_OF_PROCESS_SUPPORT"
make

%install
%qmake5_install
cd icons
make INSTALL_ROOT=%{buildroot} install

%post -p /sbin/ldconfig
%postun -p /sbin/ldconfig
