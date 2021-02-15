TEMPLATE=subdirs
SUBDIRS += \
    buteo-common \
    buteo-sync-plugin-nextcloud-backup \
    buteo-sync-plugin-nextcloud-images \
    buteo-sync-plugin-nextcloud-posts

buteo-sync-plugin-nextcloud-backup.depends = buteo-common
buteo-sync-plugin-nextcloud-images.depends = buteo-common
buteo-sync-plugin-nextcloud-posts.depends = buteo-common
