from datetime import timedelta
from os import environ, makedirs, path
import shutil, subprocess
from tempfile import mkdtemp
from unittest.mock import patch, call

from django.test import LiveServerTestCase
from django.utils.timezone import now


from delay.models import SOGroup, UNKNOWN_REPO_NAME, Deadline, PingPong


URL_PATH = '/challenge/ping_pong'

C_MODULE_FOLDER = path.join(path.dirname(path.abspath(__file__)), '..', '..', '..', 'c_module')
C_MODULE_FOLDER = path.realpath(C_MODULE_FOLDER)


class TestRunCompiled(LiveServerTestCase):
    client_file_name = 'example_client.c'
    pingpong_module_file_name = 'pingpong.c'
    binary_name = 'pp_client'

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
        shutil.copy(path.join(C_MODULE_FOLDER, self.pingpong_module_file_name),
                    path.join(folder_path, self.pingpong_module_file_name))
        shutil.copy(path.join(C_MODULE_FOLDER, self.client_file_name),
                    path.join(folder_path, self.client_file_name))

        subprocess.run(['gcc', '-Wall', self.client_file_name, '-o', path.join(folder_path, self.binary_name)], cwd=folder_path)
        return folder_path

    def tearDown(self) -> None:
        shutil.rmtree(self.folder_path)
        return super().tearDown()

    def execute_ping_pong(self, verbose_mode=True, extra_env=None, stdout=None):
        env = environ.copy()
        env['PP_URL'] = f'{self.live_server_url}{URL_PATH}'
        if verbose_mode:
            env['PP_DEBUG'] = '1'
        env.update(extra_env or {})

        kw = {'cwd': self.folder_path, 'env': env}
        if stdout:
            kw['stdout'] = stdout
        else:
            kw['capture_output'] = True
        execution = subprocess.run([f'./{self.binary_name}'], **kw)
        self.assertEqual(execution.returncode, 0)
        return execution

    def test_simple_integration(self):
        number = 123
        self.group.current_delay = number
        self.group.save()
        execution = self.execute_ping_pong()
        self.assertGreater(PingPong.objects.count(), 0)
        self.assertIn(b'OK', execution.stderr)
        self.assertIn(b'delay=%d' % number, execution.stderr)
        execution = self.execute_ping_pong(verbose_mode=False)
        self.assertEqual(b'', execution.stderr)

    def test_if_no_group_exists_with_that_repon_name_means_a_404(self):
        # c module detects a repo name, but server does not have it
        self.group.delete()
        execution = self.execute_ping_pong()
        self.assertIn(b'404', execution.stderr)
        execution = self.execute_ping_pong(verbose_mode=False)
        self.assertEqual(b'', execution.stderr)

    def test_unkown_repo_name_creates_a_default_group(self):
        # c module fails to detect a repo name, sends special string "UNKNOWN_REPO_NAME"
        other_folder_name = 'yaddayadda'
        self.folder_path = self.create_tmp_folder_and_copy_and_compile(
            other_folder_name
        )
        execution = self.execute_ping_pong(verbose_mode=True)
        self.assertIn(UNKNOWN_REPO_NAME.encode('utf-8'), execution.stderr)
        self.assertEqual(1, SOGroup.objects.filter(repo_name=UNKNOWN_REPO_NAME).count())

    def test_simple_integration_disabled(self):
        execution = self.execute_ping_pong(verbose_mode=True, extra_env={'PP_DISABLE_EASTER_EGG': '1'})
        self.assertEqual(b'Easter egg disabled. Exit\n', execution.stderr)
        self.assertEqual(PingPong.objects.count(), 0)

    def test_messages_received_and_printed(self):
        dline = Deadline.objects.create(name='test', text='this is a sample text',
                                        deadline=now() - timedelta(days=1))
        execution = self.execute_ping_pong(verbose_mode=False, stdout=subprocess.PIPE)
        self.assertIn(dline.text.encode('utf-8'), execution.stdout)
        execution = self.execute_ping_pong(verbose_mode=False, stdout=subprocess.PIPE, extra_env={'LAB_SKIP_HELP': '1'})
        self.assertNotIn(dline.text.encode('utf-8'), execution.stdout)

    def test_wrong_password_doesnt_work_and_msg_printed_in_red(self):
        RED = "\x1b[30;41m"
        NORMAL = "\x1b[0m"
        msg = 'ERROR: Wrong password. You have been penalized'
        some_password = 'some_password' + self.group.password_to_win  # to make sure it's not the correct one
        extra_env = {'EXAMPLE_CLIENT_PASSWORD': some_password}  # this envvar is only useful on example_client
        execution = self.execute_ping_pong(verbose_mode=False, stdout=subprocess.PIPE, extra_env=extra_env)
        self.assertIn(f'{RED}{msg}{NORMAL}'.encode('utf-8'),
                      execution.stdout)
        reloaded_group = SOGroup.objects.get(id=self.group.id)
        self.assertFalse(reloaded_group.challenge_won)
        # and msg can be hidden if LAB_SKIP_HELP is set
        extra_env['LAB_SKIP_HELP'] = '1'
        execution = self.execute_ping_pong(verbose_mode=False, stdout=subprocess.PIPE, extra_env=extra_env)
        self.assertNotIn(f'{RED}{msg}{NORMAL}'.encode('utf-8'),
                         execution.stdout)

    def test_correct_password_wins_and_prints_in_green(self):
        GREEN = "\x1b[32;40m"
        NORMAL = "\x1b[0m"
        extra_env = {'EXAMPLE_CLIENT_PASSWORD': self.group.password_to_win}  # this envvar is only useful on example_client
        execution = self.execute_ping_pong(verbose_mode=False, stdout=subprocess.PIPE, extra_env=extra_env)
        reloaded_group = SOGroup.objects.get(id=self.group.id)
        stamp = reloaded_group.challenge_won_timestamp
        msg = f'SUCCESS: Congratulations. Challenge won at {stamp}. No more delays.'
        self.assertIn(f'{GREEN}{msg}{NORMAL}'.encode('utf-8'),
                         execution.stdout)
        self.assertTrue(reloaded_group.challenge_won)
        extra_env['LAB_SKIP_HELP'] = '1'
        execution = self.execute_ping_pong(verbose_mode=False, stdout=subprocess.PIPE, extra_env=extra_env)
        self.assertNotIn(f'{GREEN}{msg}{NORMAL}'.encode('utf-8'),
                         execution.stdout)

    def test_pattern_can_have_only_2_digits_per_year(self):
        other_folder_name = self.group.repo_name
        assert '2024' in other_folder_name  # just make sure this tests starts from known place
        other_folder_name = other_folder_name.replace('2024', '24')
        self.folder_path = self.create_tmp_folder_and_copy_and_compile(
            other_folder_name
        )
        number = 123
        self.group.current_delay = number
        self.group.save()
        execution = self.execute_ping_pong()
        self.assertGreater(PingPong.objects.count(), 0)
        self.assertIn(b'OK', execution.stderr)
        self.assertIn(b'delay=%d' % number, execution.stderr)
        execution = self.execute_ping_pong(verbose_mode=False)
        self.assertEqual(b'', execution.stderr)

class TestRunCompiledObfuscated(TestRunCompiled):
    client_file_name = 'example_client_obfuscated.c'
    pingpong_module_file_name = 'obfuscated.c'
    binary_name = 'pp_obfuscated_client'
