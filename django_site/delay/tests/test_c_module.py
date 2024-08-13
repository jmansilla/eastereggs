from os import environ, makedirs, path
import shutil, subprocess
from tempfile import mkdtemp
from unittest.mock import patch, call

from django.shortcuts import get_object_or_404
from django.test import TestCase, LiveServerTestCase


from delay.generate_password import xor_encrypt
from delay.models import SOGroup
from delay.views import decrypt_group_name, UNKWON_USERID, HERE_IS_THE_PASSWORD_TEMPLATE


URL_PATH = '/delay/ping_pong'

C_MODULE_FOLDER = path.join(path.dirname(path.abspath(__file__)), '..', '..', '..', 'c_module')
C_MODULE_FOLDER = path.realpath(C_MODULE_FOLDER)


class TestRunCompiled(LiveServerTestCase):
    def setUp(self) -> None:
        from delay.tests.test_delay import REPO_NAME  # if not imported here, test runner may get crazy
        self.group = SOGroup.objects.create(repo_name=REPO_NAME)
        self.folder_path = self.create_tmp_folder_and_copy_and_compile(REPO_NAME)
        return super().setUp()

    def create_tmp_folder_and_copy_and_compile(self, folder_name):
        temp_container_folder = mkdtemp()
        self.addCleanup(shutil.rmtree, temp_container_folder)
        folder_path = path.join(temp_container_folder, folder_name)
        makedirs(folder_path, exist_ok=True)
        shutil.copy(path.join(C_MODULE_FOLDER, 'pingpong.c'), path.join(folder_path, 'pingpong.c'))
        shutil.copy(path.join(C_MODULE_FOLDER, 'example_client.c'), path.join(folder_path, 'example_client.c'))
        self.binary_name = 'pp_client'
        subprocess.run(['gcc', '-Wall', 'example_client.c', '-o', path.join(folder_path, self.binary_name)], cwd=folder_path)
        return folder_path

    def tearDown(self) -> None:
        shutil.rmtree(self.folder_path)
        return super().tearDown()

    def execute_ping_pong(self, verbose_mode=True, extra_env=None):
        env = environ.copy()
        env['PP_URL'] = f'{self.live_server_url}{URL_PATH}'
        if verbose_mode:
            env['PP_DEBUG'] = '1'
        env.update(extra_env or {})
        return subprocess.run([f'./{self.binary_name}'], cwd=self.folder_path, capture_output=True, env=env)

    def test_simple_integration(self):
        number = 123
        self.group.current_delay = 123
        self.group.save()
        execution = self.execute_ping_pong()
        self.assertEqual(execution.returncode, 0)
        self.assertIn(b'OK', execution.stderr)
        self.assertIn(b'delay=%d' % number, execution.stderr)
        execution = self.execute_ping_pong(verbose_mode=False)
        self.assertEqual(b'', execution.stderr)

    def test_no_group_means_a_404(self):
        self.group.delete()
        execution = self.execute_ping_pong()
        self.assertEqual(execution.returncode, 0)
        self.assertIn(b'404', execution.stderr)
        execution = self.execute_ping_pong(verbose_mode=False)
        self.assertEqual(execution.returncode, 0)
        self.assertEqual(b'', execution.stderr)

    def test_simple_integration(self):
        execution = self.execute_ping_pong(verbose_mode=True, extra_env={'PP_DISABLE_EASTER_EGG': '1'})
        self.assertEqual(execution.returncode, 0)
        self.assertEqual(b'Easter egg disabled. Exit\n', execution.stderr)
