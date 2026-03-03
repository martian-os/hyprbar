#!/usr/bin/env python3
"""
Mock D-Bus StatusNotifierWatcher for testing tray widget
"""
import sys
import os
import dbus
import dbus.service
import dbus.mainloop.glib
from gi.repository import GLib

class MockStatusNotifierWatcher(dbus.service.Object):
    def __init__(self, bus, items):
        """
        items: list of dicts with 'service', 'path', 'icon_name', 'icon_pixmap'
        """
        self.bus_name = dbus.service.BusName('org.kde.StatusNotifierWatcher', bus=bus)
        super().__init__(self.bus_name, '/StatusNotifierWatcher')
        self.items = items
        
    @dbus.service.method('org.kde.StatusNotifierWatcher',
                         out_signature='as')
    def RegisteredStatusNotifierItems(self):
        """Return list of registered tray items"""
        result = []
        for item in self.items:
            result.append(f"{item['service']}@{item['path']}")
        return result

class MockStatusNotifierItem(dbus.service.Object):
    def __init__(self, bus, service_name, object_path, icon_name=None, icon_pixmap=None):
        self.bus_name = dbus.service.BusName(service_name, bus=bus)
        super().__init__(self.bus_name, object_path)
        self.icon_name = icon_name
        self.icon_pixmap = icon_pixmap
        
    @dbus.service.method('org.freedesktop.DBus.Properties',
                         in_signature='ss', out_signature='v')
    def Get(self, interface, prop):
        if prop == 'IconName' and self.icon_name:
            return self.icon_name
        elif prop == 'IconPixmap' and self.icon_pixmap:
            return self.icon_pixmap
        else:
            raise dbus.exceptions.DBusException(
                f'Property {prop} not found',
                name='org.freedesktop.DBus.Error.UnknownProperty'
            )

def main():
    if len(sys.argv) < 2:
        print("Usage: mock_dbus_server.py <config_file>")
        print("Config file is JSON with mock item definitions")
        sys.exit(1)
    
    config_file = sys.argv[1]
    
    # Default items if no config
    if not os.path.exists(config_file):
        items = [
            {
                'service': ':1.100',
                'path': '/StatusNotifierItem',
                'icon_name': 'test-icon-symbolic',
                'icon_pixmap': None
            }
        ]
    else:
        import json
        with open(config_file) as f:
            items = json.load(f)
    
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)
    session_bus = dbus.SessionBus()
    
    # Create watcher
    watcher = MockStatusNotifierWatcher(session_bus, items)
    
    # Create items
    item_objects = []
    for item in items:
        obj = MockStatusNotifierItem(
            session_bus,
            item['service'],
            item['path'],
            item.get('icon_name'),
            item.get('icon_pixmap')
        )
        item_objects.append(obj)
    
    print(f"Mock D-Bus StatusNotifierWatcher started with {len(items)} items", flush=True)
    
    loop = GLib.MainLoop()
    try:
        loop.run()
    except KeyboardInterrupt:
        print("Mock D-Bus stopped", flush=True)
        sys.exit(0)

if __name__ == '__main__':
    main()
