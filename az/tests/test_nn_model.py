"""Shape/sanity tests for SuperChessNet's forward pass."""

import numpy as np
import torch

from az.encoding import NUM_PLANES, POLICY_SIZE
from az.nn.inference import NetworkWrapper
from az.nn.model import SuperChessNet


def test_forward_pass_shapes():
    model = SuperChessNet(num_blocks=2, channels=16)
    batch = torch.zeros(3, NUM_PLANES, 8, 8)
    policy_logits, value = model(batch)
    assert policy_logits.shape == (3, POLICY_SIZE)
    assert value.shape == (3, 1)


def test_value_head_is_bounded_by_tanh():
    model = SuperChessNet(num_blocks=2, channels=16)
    batch = torch.randn(8, NUM_PLANES, 8, 8)
    _, value = model(batch)
    assert torch.all(value >= -1.0) and torch.all(value <= 1.0)


def test_network_wrapper_predict_matches_expected_shapes():
    model = SuperChessNet(num_blocks=2, channels=16)
    wrapper = NetworkWrapper(model, device="cpu")
    batch = np.zeros((4, NUM_PLANES, 8, 8), dtype=np.float32)
    policy_logits, values = wrapper.predict(batch)
    assert policy_logits.shape == (4, POLICY_SIZE)
    assert values.shape == (4,)
