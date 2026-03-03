#!/usr/bin/env python3
"""
Mock Hyprland IPC server for testing HyprlandWidget.

Creates two Unix sockets that simulate Hyprland's IPC:
1. .socket.sock - Command socket (responds to j/workspaces, j/activeworkspace)
2. .socket2.sock - Event socket (sends workspace events)

Usage:
    python3 mock_hyprland_server.py /tmp/mock_hypr/test_instance
"""

import socket
import os
import sys
import threading
import time
import json

class MockHyprlandServer:
    def __init__(self, socket_dir):
        self.socket_dir = socket_dir
        self.command_socket_path = os.path.join(socket_dir, '.socket.sock')
        self.event_socket_path = os.path.join(socket_dir, '.socket2.sock')
        self.running = False
        
        # Mock workspace data
        self.workspaces = [
            {"id": 1, "name": "1", "windows": 3},
            {"id": 2, "name": "2", "windows": 1},
            {"id": 3, "name": "3", "windows": 0},
            {"id": 4, "name": "4", "windows": 0},
        ]
        self.active_workspace_id = 1
        
    def start(self):
        """Start the mock server"""
        os.makedirs(self.socket_dir, exist_ok=True)
        self.running = True
        
        # Start command socket thread
        self.command_thread = threading.Thread(target=self._run_command_socket, daemon=True)
        self.command_thread.start()
        
        # Start event socket thread
        self.event_thread = threading.Thread(target=self._run_event_socket, daemon=True)
        self.event_thread.start()
        
        print(f"Mock Hyprland server started at {self.socket_dir}", file=sys.stderr)
        
    def stop(self):
        """Stop the mock server"""
        self.running = False
        
        # Clean up sockets
        for path in [self.command_socket_path, self.event_socket_path]:
            if os.path.exists(path):
                os.unlink(path)
                
    def _run_command_socket(self):
        """Run the command socket (responds to queries)"""
        # Clean up old socket
        if os.path.exists(self.command_socket_path):
            os.unlink(self.command_socket_path)
            
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.bind(self.command_socket_path)
        sock.listen(1)
        sock.settimeout(0.1)
        
        while self.running:
            try:
                conn, _ = sock.accept()
                self._handle_command(conn)
            except socket.timeout:
                continue
            except Exception as e:
                print(f"Command socket error: {e}", file=sys.stderr)
                
        sock.close()
        
    def _handle_command(self, conn):
        """Handle a command from client"""
        try:
            data = conn.recv(1024).decode('utf-8').strip()
            
            if data == 'j/workspaces':
                response = json.dumps(self.workspaces) + '\n'
                conn.sendall(response.encode('utf-8'))
            elif data == 'j/activeworkspace':
                active = next((w for w in self.workspaces if w['id'] == self.active_workspace_id), self.workspaces[0])
                response = json.dumps(active) + '\n'
                conn.sendall(response.encode('utf-8'))
            else:
                conn.sendall(b'{}\n')
                
        except Exception as e:
            print(f"Error handling command: {e}", file=sys.stderr)
        finally:
            conn.close()
            
    def _run_event_socket(self):
        """Run the event socket (sends workspace change events)"""
        # Clean up old socket
        if os.path.exists(self.event_socket_path):
            os.unlink(self.event_socket_path)
            
        sock = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
        sock.bind(self.event_socket_path)
        sock.listen(5)
        sock.settimeout(0.1)
        
        clients = []
        
        while self.running:
            try:
                conn, _ = sock.accept()
                clients.append(conn)
            except socket.timeout:
                pass
            except Exception as e:
                print(f"Event socket error: {e}", file=sys.stderr)
                
        sock.close()
        
    def send_workspace_event(self, workspace_id):
        """Simulate a workspace change event"""
        self.active_workspace_id = workspace_id
        # In real implementation, would send to connected clients

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Usage: mock_hyprland_server.py <socket_dir>", file=sys.stderr)
        sys.exit(1)
        
    server = MockHyprlandServer(sys.argv[1])
    server.start()
    
    try:
        # Keep running until interrupted
        while True:
            time.sleep(1)
    except KeyboardInterrupt:
        server.stop()
        print("\nMock server stopped", file=sys.stderr)
