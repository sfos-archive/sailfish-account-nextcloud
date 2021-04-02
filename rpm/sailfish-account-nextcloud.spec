Name: sailfish-account-nextcloud
License: Proprietary
Version: 0.1.13
Release: 1
Source0: %{name}-%{version}.tar.bz2
Summary: Account plugin for Nextcloud
BuildRequires: qt5-qmake
BuildRequires: sailfish-svg2png
BuildRequires: pkgconfig(Qt5Core)
BuildRequires: pkgconfig(Qt5DBus)
BuildRequires: pkgconfig(Qt5Sql)
BuildRequires: pkgconfig(Qt5Network)
BuildRequires: pkgconfig(Qt5Gui)
BuildRequires: pkgconfig(Qt5Qml)
BuildRequires: pkgconfig(mlite5)
BuildRequires: pkgconfig(buteosyncfw5) >= 0.10.0
BuildRequires: pkgconfig(libsignon-qt5)
BuildRequires: pkgconfig(accounts-qt5)
BuildRequires: pkgconfig(socialcache)
BuildRequires: pkgconfig(libsailfishkeyprovider)
BuildRequires: pkgconfig(sailfishaccounts)
BuildRequires: pkgconfig(nemotransferengine-qt5)
BuildRequires: qt5-qttools
BuildRequires: qt5-qttools-linguist
Requires(post): %{_libexecdir}/manage-groups
Requires(postun): %{_libexecdir}/manage-groups

%description
%{summary}.

%post
/sbin/ldconfig
%{_libexecdir}/manage-groups add account-nextcloud || :

%postun
/sbin/ldconfig
if [ "$1" -eq 0 ]; then
    %{_libexecdir}/manage-groups remove account-nextcloud || :
fi

%files
%defattr(-,root,root,-)
%{_libdir}/libnextcloudcommon.so.*
%exclude %{_libdir}/libnextcloudcommon.so
%{_libdir}/libnextcloudbuteocommon.so.*
%exclude %{_libdir}/libnextcloudbuteocommon.so
%{_datadir}/accounts/providers/nextcloud.provider
%{_datadir}/accounts/services/nextcloud-backup.service
%{_datadir}/accounts/services/nextcloud-caldav.service
%{_datadir}/accounts/services/nextcloud-carddav.service
%{_datadir}/accounts/services/nextcloud-images.service
%{_datadir}/accounts/services/nextcloud-posts.service
%{_datadir}/accounts/services/nextcloud-sharing.service
%{_datadir}/accounts/ui/nextcloud.qml
%{_datadir}/accounts/ui/nextcloud-settings.qml
%{_datadir}/accounts/ui/nextcloud-update.qml
# TODO: only install the appropriate scale factor icons for the build target?  How?
%{_datadir}/themes/sailfish-default/meegotouch/z1.0/icons/graphic-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.0/icons/graphic-m-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.0/icons/graphic-s-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.0/icons/icon-l-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.0/icons/icon-m-file-folder-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.25/icons/graphic-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.25/icons/graphic-m-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.25/icons/graphic-s-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.25/icons/icon-l-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.25/icons/icon-m-file-folder-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5/icons/graphic-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5/icons/graphic-m-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5/icons/graphic-s-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5/icons/icon-l-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5/icons/icon-m-file-folder-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5-large/icons/graphic-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5-large/icons/graphic-m-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5-large/icons/graphic-s-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5-large/icons/icon-l-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.5-large/icons/icon-m-file-folder-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.75/icons/graphic-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.75/icons/graphic-m-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.75/icons/graphic-s-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.75/icons/icon-l-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z1.75/icons/icon-m-file-folder-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z2.0/icons/graphic-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z2.0/icons/graphic-m-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z2.0/icons/graphic-s-service-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z2.0/icons/icon-l-nextcloud.png
%{_datadir}/themes/sailfish-default/meegotouch/z2.0/icons/icon-m-file-folder-nextcloud.png


%package -n buteo-sync-plugin-nextcloud-posts
Summary:   Provides synchronisation of posts blobs with Nextcloud
Requires: %{name} = %{version}-%{release}
Requires: buteo-syncfw-qt5-msyncd
Requires: systemd
Requires(post): systemd

%description -n buteo-sync-plugin-nextcloud-posts
Provides synchronisation of posts blobs with Nextcloud.

%files -n buteo-sync-plugin-nextcloud-posts
%defattr(-,root,root,-)
%{_libdir}/buteo-plugins-qt5/oopp/libnextcloud-posts-client.so
%config %{_sysconfdir}/buteo/profiles/client/nextcloud-posts.xml
%config %{_sysconfdir}/buteo/profiles/sync/nextcloud.Posts.xml


%package -n eventsview-extensions-nextcloud
Summary:   Provides integration of Nextcloud notifications into Events view
Requires:      lipstick-jolla-home-qt5-components >= 1.2.50

%description -n eventsview-extensions-nextcloud
Provides integration of Nextcloud notifications into Events view

%files -n eventsview-extensions-nextcloud
%defattr(-,root,root,-)
%{_datadir}/translations/eventsview-nextcloud_eng_en.qm
%{_libdir}/qt5/qml/com/jolla/eventsview/nextcloud/*
%{_datadir}/lipstick/eventfeed/*


%package -n eventsview-extensions-nextcloud-ts-devel
Summary:  Translation source for Events Nextcloud plugin
Requires: eventsview-extensions-nextcloud

%description -n eventsview-extensions-nextcloud-ts-devel
Translation source for Events Nextcloud plugin.

%files -n eventsview-extensions-nextcloud-ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/eventsview-nextcloud.ts


%package -n buteo-sync-plugin-nextcloud-backup
Summary:   Provides synchronisation of backup/restore blobs with Nextcloud
Requires: %{name} = %{version}-%{release}
Requires: buteo-syncfw-qt5-msyncd
Requires: systemd
Requires(post): systemd

%description -n buteo-sync-plugin-nextcloud-backup
Provides synchronisation of backup/restore blobs with Nextcloud.

%files -n buteo-sync-plugin-nextcloud-backup
%defattr(-,root,root,-)
%{_libdir}/buteo-plugins-qt5/oopp/libnextcloud-backup-client.so
%{_libdir}/buteo-plugins-qt5/oopp/libnextcloud-backupquery-client.so
%{_libdir}/buteo-plugins-qt5/oopp/libnextcloud-backuprestore-client.so
%config %{_sysconfdir}/buteo/profiles/client/nextcloud-backup.xml
%config %{_sysconfdir}/buteo/profiles/client/nextcloud-backupquery.xml
%config %{_sysconfdir}/buteo/profiles/client/nextcloud-backuprestore.xml
%config %{_sysconfdir}/buteo/profiles/sync/nextcloud.Backup.xml
%config %{_sysconfdir}/buteo/profiles/sync/nextcloud.BackupQuery.xml
%config %{_sysconfdir}/buteo/profiles/sync/nextcloud.BackupRestore.xml


%package -n buteo-sync-plugin-nextcloud-images
Summary:   Provides synchronisation of gallery images with Nextcloud
Requires: %{name} = %{version}-%{release}
Requires: buteo-syncfw-qt5-msyncd
Requires: systemd
Requires(post): systemd

%description -n buteo-sync-plugin-nextcloud-images
Provides synchronisation of gallery images with Nextcloud.

%files -n buteo-sync-plugin-nextcloud-images
%defattr(-,root,root,-)
%{_libdir}/buteo-plugins-qt5/oopp/libnextcloud-images-client.so
%config %{_sysconfdir}/buteo/profiles/client/nextcloud-images.xml
%config %{_sysconfdir}/buteo/profiles/sync/nextcloud.Images.xml


%package -n jolla-gallery-extension-nextcloud
Summary:   Provides integration of Nextcloud images into Gallery application
Requires: sailfish-components-gallery-qt5 >= 1.1.9
Requires: sailfish-components-filemanager >= 0.2.17

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
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/NextcloudDirectoryItem.qml
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/NextcloudPhotoListPage.qml
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/NextcloudFullscreenPhotoPage.qml
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/NextcloudImageDetailsPage.qml
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/qmldir
%{_libdir}/qt5/qml/com/jolla/gallery/nextcloud/libjollagallerynextcloudplugin.so
%dir %{_libdir}/qt5/qml/com/jolla/gallery/nextcloud


%package -n jolla-gallery-extension-nextcloud-ts-devel
Summary:  Translation source for Gallery Nextcloud plugin
Requires: jolla-gallery-extension-nextcloud

%description -n jolla-gallery-extension-nextcloud-ts-devel
Translation source for Gallery Nextcloud plugin.

%files -n jolla-gallery-extension-nextcloud-ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/gallery-extension-nextcloud.ts


%package -n transferengine-plugin-nextcloud
Summary: Nextcloud file sharing plugin for Transfer Engine
Requires: sailfishsilica-qt5 >= 1.1.108
Requires: declarative-transferengine-qt5 >= 0.3.13
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
Requires: transferengine-plugin-nextcloud

%description -n transferengine-plugin-nextcloud-ts-devel
Translation source for Nextcloud Transfer Engine share plugin.

%files -n transferengine-plugin-nextcloud-ts-devel
%defattr(-,root,root,-)
%{_datadir}/translations/source/sailfish_transferengine_plugin_nextcloud.ts


%package features-all
Summary:   Meta package to include all Nextcloud account features
Requires: %{name} = %{version}-%{release}
Requires: transferengine-plugin-nextcloud
Requires: jolla-gallery-extension-nextcloud
Requires: eventsview-extensions-nextcloud
Requires: buteo-sync-plugin-nextcloud-images
Requires: buteo-sync-plugin-nextcloud-backup
Requires: buteo-sync-plugin-nextcloud-posts

%description features-all
This package is here to include all Nextcloud account
features to image (e.g. sharing, image sync, backups, etc).

%files features-all
# Empty as this is meta package.


%prep
%setup -q -n %{name}-%{version}

%build
%qmake5 "VERSION=%{version}"
%make_build

%install
%qmake5_install
cd icons
make INSTALL_ROOT=%{buildroot} install
