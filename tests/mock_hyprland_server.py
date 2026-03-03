#!/usr/bin/env python3
"""
Mock Hyprland IPC server for testing - synchronous version with proper cleanup.

Creates Unix sockets that simulate Hyprland's IPC protocol.
Designed to be started/stopped quickly for unit tests.
"""

import socket
import os
import sys
import json
import signal
import threading
import time

class MockHyprlandServer:
    def __init__(self, socket_dir):
        self.socket_dir = socket_dir
        self.command_path = os.path.join(socket_dir, '.socket.sock')
        self.event_path = os.path.join(socket_dir, '.socket2.sock')
        self.running = False
        self.command_sock = None
        self.event_sock = None
        
        # Mock data
        self.workspaces = [
            {"id": 1, "name": "1", "windows": 3},
            {"id": 2, "name": "2", "windows": 1},
            {"id": 3, "name": "3", "windows": 0},
        ]
        self.active_id = 1
        
    def start(self):
        """Start server (blocks)"""
        os.makedirs(self.socket_dir, exist_ok=True)
        self.running = True
        
        # Setup signal handler for clean shutdown
        signal.signal(signal.SIGTERM, lambda s, f: self.stop())
        
        # Start command socket in thread
        cmd_thread = threading.Thread(target=self._run_command_socket, daemon=False)
        cmd_thread.start()
        
        # Start event socket in thread
        evt_thread = threading.Thread(target=self._run_event_socket, daemon=False)
        evt_thread.start()
        
        print(f"Mock Hyprland ready: {self.socket_dir}", file=sys.stderr, flush=True)
        
        # Keep main thread alive
        try:
            while self.running:
                time.sleep(0.1)
        except KeyboardInterrupt:
            pass
        finally:
            self.stop()
            
    def stop(self):
        """Stop server and clean up"""
        self.running = False
        
        # Close sockets
        if self.command_sock:
            try:
                self.command_sock.close()
            except:
                pass
        if self.event_sock:
            try:
                self.event_sock.close()
            except:
                pass
                
        # Remove socket files
        for path in [self.command_path, self.event_path]:
            try:
                if os.path.exists(path):
                    os.unlink(path)
            except:
                pass
                
        print("Mock Hyprland stopped", file=sys.stderr, flush=True)
        
    def _run_command_socket(self):
        """Handle command requests"""
        if os.path.exists(self.command_path):
            os.unlink(self.command_path)
            
        self.command_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.command_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.command_sock.bind(self.command_path)
        self.command_sock.listen(5)
        self.command_sock.settimeout(0.1)
        
        while self.running:
            try:
                conn, _ = self.command_sock.accept()
                conn.settimeout(1.0)
                try:
                    data = conn.recv(1024).decode('utf-8').strip()
                    
                    if data == 'j/workspaces':
                        response = json.dumps(self.workspaces)
                    elif data == 'j/activeworkspace':
                        active = next((w for w in self.workspaces if w['id'] == self.active_id), self.workspaces[0])
                        response = json.dumps(active)
                    else:
                        response = "{}"
                        
                    conn.sendall(response.encode('utf-8'))
                finally:
                    conn.close()
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    print(f"Command socket error: {e}", file=sys.stderr)
                    
    def _run_event_socket(self):
        """Handle event stream (just accept connections, don't send events)"""
        if os.path.exists(self.event_path):
            os.unlink(self.event_path)
            
        self.event_sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        self.event_sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.event_sock.bind(self.event_path)
        self.event_sock.listen(5)
        self.event_sock.settimeout(0.1)
        
        clients = []
        
        while self.running:
            try:
                conn, _ = self.event_sock.accept()
                clients.append(conn)
            except socket.timeout:
                continue
            except Exception as e:
                if self.running:
                    print(f"Event socket error: {e}", file=sys.stderr)
                    
        # Clean up clients
        for conn in clients:
            try:
                conn.close()
            except:
                pass

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: mock_hyprland_server.py <socket_dir>", file=sys.stderr)
        sys.exit(1)
        
    server = MockHyprlandServer(sys.argv[1])
    server.start()
