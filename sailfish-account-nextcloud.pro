TEMPLATE=subdirs
SUBDIRS += \
    common \
    settings \
    buteo-plugins \
    transferengine-plugins \
    gallery-extension-plugins \
    eventsview-plugins \
    icons

buteo-plugins.depends = common
transferengine-plugins.depends = common
gallery-extension-plugins.depends = common
eventsview-plugins.depends = common

OTHER_FILES+=rpm/sailfish-account-nextcloud.spec
