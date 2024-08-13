from unittest.mock import patch, call

from django.shortcuts import get_object_or_404
from django.test import TestCase

from delay.generate_password import xor_encrypt
from delay.models import SOGroup
from delay.views import decrypt_group_name, UNKWON_USERID, HERE_IS_THE_PASSWORD_TEMPLATE


URL_PATH = '/delay/ping_pong'


class TestDecryptGroupName(TestCase):

    def test_if_empty_string_returns_none(self):
        self.assertIsNone(decrypt_group_name(''))

    def test_hex_string_and_xor_returns_original_string(self):
        original = 'so2024lab1g05'
        xored = xor_encrypt(original, 5)
        as_hex = xored.encode('utf-8').hex()
        self.assertEqual(decrypt_group_name(as_hex), original)

    def test_exhaustive_group_numbers_from_00_to_99(self):
        for i in range(100):
            original = f'so2024lab1g{i:02}'
            xored = xor_encrypt(original, salt=i % 50)
            as_hex = xored.encode('utf-8').hex()
            self.assertEqual(decrypt_group_name(as_hex), original)


class TestPingPongViewLookupForGroup(TestCase):
    # testing how the md5 param is used to find the group to use within the view
    def test_with_no_args_returns_a_404(self):
        response = self.client.get(URL_PATH, follow=True)
        self.assertEqual(response.status_code, 404)

    def test_arg_md5_is_decrypted_and_used_to_find_group(self):
        with patch('delay.views.decrypt_group_name') as mock_decrypt:
            md5 = '123'
            decrypted = '123-decrypted'
            mock_decrypt.return_value = decrypted
            original_get_or_404 = get_object_or_404
            with patch('delay.views.get_object_or_404') as mock_get_or_404:
                mock_get_or_404.side_effect = get_object_or_404
                response = self.client.get(f'{URL_PATH}?md5={md5}', follow=True)
                mock_decrypt.assert_called_once_with(md5)
                mock_get_or_404.assert_called_once_with(SOGroup, repo_name=decrypted)


class TestPingPongView(TestCase):
    def setUp(self) -> None:
        from delay.tests.test_delay import REPO_NAME  # if not imported here, test runner may get crazy
        self.group = SOGroup.objects.create(repo_name=REPO_NAME)
        patcher = patch('delay.views.decrypt_group_name')
        self.mock_decrypt = patcher.start()
        self.addCleanup(patcher.stop)
        self.mock_decrypt.return_value = REPO_NAME
        return super().setUp()

    def test_handle_ping_is_called_with_user_id(self):
        # detailed behaviour of handle_delay is tested in test_delay.py
        original_method = self.group.handle_ping
        with patch.object(SOGroup, 'handle_ping') as mock_handle_ping:
            mock_handle_ping.side_effect = original_method
            response = self.client.get(f'{URL_PATH}?md5=bla', follow=True)
            mock_handle_ping.assert_called_once_with(UNKWON_USERID)
            some_uid = 'some-uid'
            response = self.client.get(f'{URL_PATH}?md5=bla&user_id={some_uid}', follow=True)
            self.assertEqual(mock_handle_ping.call_args_list[-1], call(some_uid))

    def test_response_is_plain_text(self):
        response = self.client.get(f'{URL_PATH}?md5=bla', follow=True)
        self.assertEqual(response['content-type'], 'text/plain')
        last_ping_pong = self.group.pingpong_set.last()
        lines = response.content.splitlines()
        self.assertEqual(lines[0], b'OK')
        self.assertEqual(lines[1], b'delay=%d' % last_ping_pong.delay_recommended)
        self.assertEqual(lines[2], b'pp_id=%d' % last_ping_pong.id)
        self.assertEqual(lines[3], b'repo_name=%s' % self.group.repo_name.encode('utf-8'))
        offset = 4
        for help_line in (HERE_IS_THE_PASSWORD_TEMPLATE % (self.group.password_to_win, self.group.password_to_win)).splitlines():
            self.assertEqual(lines[offset], help_line.encode('utf-8'))
            offset += 1

    def test_extra_messages_are_in_the_response(self):
        extra_message = 'Trojan Horse\n'
        original_method = self.group.handle_ping
        def with_extra_message(uid):
            pp, msgs = original_method(uid)
            msgs += extra_message
            return pp, msgs
        with patch.object(SOGroup, 'handle_ping') as mock_handle_ping:
            mock_handle_ping.side_effect = with_extra_message
            response = self.client.get(f'{URL_PATH}?md5=bla', follow=True)
            self.assertIn(extra_message.encode('utf-8'), response.content)

    def test_if_closing_id_is_provided_the_view_closes_the_ping_pong(self):
        # detailed behaviour of handle_close is tested in test_delay.py
        closing_id = 123
        with patch.object(SOGroup, 'handle_close') as mock_handle_close:
            mock_handle_close.return_value = None
            response = self.client.get(f'{URL_PATH}?md5=bla&closing_pp_id={closing_id}', follow=True)
            mock_handle_close.assert_called_once_with(str(closing_id))
            # as handle_close returned None, response shall be 404
            self.assertEqual(response.status_code, 404)
            # let's now create one and return it
            pp, _ = self.group.handle_ping(UNKWON_USERID)
            mock_handle_close.return_value = pp
            response = self.client.get(f'{URL_PATH}?md5=bla&closing_pp_id={pp.id}', follow=True)
            self.assertEqual(response.status_code, 200)

    def test_if_password_is_provided_the_view_attempts_to_win(self):
        # detailed behaviour of handle_win_attempt is tested in test_delay.py
        original_method = self.group.handle_win_attempt

        with patch.object(SOGroup, 'handle_win_attempt') as mock_handle_win_attempt:
            mock_handle_win_attempt.side_effect = original_method
            response = self.client.get(f'{URL_PATH}?md5=bla&password_to_win={self.group.password_to_win}', follow=True)
            mock_handle_win_attempt.assert_called_once_with(UNKWON_USERID, self.group.password_to_win)
            self.assertEqual(response.status_code, 200)
